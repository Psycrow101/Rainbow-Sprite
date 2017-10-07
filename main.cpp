#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum SPRTYPE
{
	VP_PARALLEL_UPRIGHT,
	FACING_UPRIGHT,
	VP_PARALLEL,
	ORIENTED,
	VP_PARALLEL_ORIENTED
};

enum SPRTEXTFORMAT
{
	SPR_NORMAL,
	SPR_ADDITIVE,
	SPR_INDEXALPHA,
	SPR_ALPHTEST
};

typedef struct sprheader {
	unsigned spriteType;
	unsigned textureFormat;
	float boundingRadius;
	unsigned maxFrameWidth;
	unsigned maxFrameheight;
	unsigned numFrames;
	float beamLength;
	unsigned synchronizationType;
} sprheader;

typedef struct frameheader {
	unsigned frameGroup;
	int frameOriginX;
	int frameOriginY;
	unsigned frameWidth;
	unsigned frameHeight;
} frameheader;

typedef struct color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} color;

int main(int args_n, char **args)
{
	if (args_n < 3)
	{
		printf("Not enough arguments\nHow to use:\nsprite_name.spr color_row_value \"r g b\" \"r g b\" . . .\n");
		return 1;
	}

	FILE *fp;

	fp = fopen(args[1], "rb");
	if (!fp)
	{
		printf("Unable to open file\n");
		return 1;
	}
	fseek(fp, 0, SEEK_SET);

	unsigned fileId;
	fread(&fileId, 1, 4, fp);

	if (fileId != 1347634249)
	{
		fclose(fp);
		printf("Invalid fileId\n");
		return 1;
	}

	unsigned spriteVersion;
	fread(&spriteVersion, 1, 4, fp);

	if (spriteVersion != 2)
	{
		fclose(fp);
		printf("Invalid Sprite Version\n");
		return 1;
	}

	sprheader sprHdr;
	fread(&sprHdr, 1, sizeof(sprHdr), fp);

	int isIgnoreTransColor;
	switch (sprHdr.textureFormat)
	{
	case SPR_NORMAL:
		isIgnoreTransColor = 0;
		break;
	case SPR_ALPHTEST:
		isIgnoreTransColor = 1;
		break;
	default:
		fclose(fp);
		printf("This texture format is not supported in the current version\n");
		return 1;
	}

	if (sprHdr.numFrames > 1)
		printf("[Warning] Animated sprites in this version are not fully supported\nOnly the first frame is taken\n");

	unsigned short palSize;
	fread(&palSize, 1, 2, fp);

	color *pal = (color *)malloc(sizeof(color) * palSize);
	color transColor;

	if (isIgnoreTransColor)
	{
		fread(pal, palSize - 1, 3, fp);
		fread(&transColor, 1, 3, fp);
	}
	else
		fread(pal, palSize, 3, fp);

	double brightness;

	int i, j, k;

	for (i = 0; i < palSize; i++)
	{
		if (isIgnoreTransColor && i == palSize - 1)
			break;

		brightness = 0.2126 * (pal[i].r / 255.0) + 0.7152 * (pal[i].g / 255.0) + 0.0722 * (pal[i].b / 255.0);
		pal[i].r = pal[i].g = pal[i].b = (int)(brightness * 255);
	}

	int colorRowSize = atoi(args[2]);

	if (colorRowSize > 255 || colorRowSize < 2)
	{
		fclose(fp);
		printf("Invalid color row value\nMust be less 255 and more 2\n");
		return 1;
	}
	if (colorRowSize > 255 || colorRowSize < 2)
	{
		fclose(fp);
		printf("Invalid color row value\nMust be less 255 and more 2\n");
		return 1;
	}

	unsigned char *fixIndexes = (unsigned char *)malloc(palSize);
	unsigned char *colorRowBlocks = (unsigned char *)calloc(colorRowSize, 1);
	unsigned *koefColors = (unsigned *)calloc(colorRowSize, sizeof(unsigned));

	int colorRowNum = (isIgnoreTransColor ? 255 : 256) / colorRowSize;

	for (i = 0; i < palSize; i++)
	{
		if (isIgnoreTransColor && i == palSize - 1)
		{
			fixIndexes[palSize - 1] = 255;
			break;
		}

		for (j = 0; j < colorRowSize; j++)
		{
			if (j * colorRowNum <= pal[i].r && ((j + 1) * colorRowNum > pal[i].r))
			{
				fixIndexes[i] = j;
				colorRowBlocks[j]++;
				koefColors[j] += pal[i].r;
			}
		}

		if (pal[i].r >= colorRowSize * colorRowNum)
		{
			fixIndexes[i] = colorRowSize - 1;
			colorRowBlocks[colorRowSize - 1]++;
			koefColors[colorRowSize - 1] += pal[i].r;
		}
	}

	for (i = 0; i < colorRowSize; i++)
		koefColors[i] = colorRowBlocks[i] ? koefColors[i] / colorRowBlocks[i] : 0;

	free(pal);
	free(colorRowBlocks);

	frameheader frameHdr;
	fread(&frameHdr, 1, sizeof(frameHdr), fp);

	unsigned char *frame = (unsigned char *)malloc(frameHdr.frameWidth * frameHdr.frameHeight);
	fread(frame, frameHdr.frameWidth * frameHdr.frameHeight, 1, fp);

	fclose(fp);

	unsigned short keysNum = 2;
	color colorKeys[2];
	colorKeys[0].r = 255;
	colorKeys[0].g = 0;
	colorKeys[0].b = 0;
	colorKeys[1].r = 0;
	colorKeys[1].g = 0;
	colorKeys[1].b = 255;

	unsigned char colorGradientSize = keysNum > 2 ? colorRowNum / keysNum : colorRowNum;
	unsigned frameNum = colorGradientSize * keysNum - keysNum;

	if (frameNum > (unsigned)(isIgnoreTransColor ? 255 : 256) || frameNum < 1)
	{
		fclose(fp);
		printf("Try to set a smaller color row value or color keys num...\n");
		free(fixIndexes);
		free(koefColors);
		free(frame);
		return 1;
	}

	char newFileName[30];
	char additionName[] = "_new.spr";
	strcpy(newFileName, args[1]);
	newFileName[strlen(newFileName) - 4] = '\0';
	strcat(newFileName, additionName);

	fp = fopen(newFileName, "wb");
	if (!fp)
	{
		printf("Unable to save file\n");
		free(fixIndexes);
		free(koefColors);
		free(frame);
		return 1;
	}

	fseek(fp, 0, SEEK_SET);

	fwrite("IDSP", 4, 1, fp);
	fwrite(&spriteVersion, 1, 4, fp);

	sprHdr.numFrames = frameNum;
	fwrite(&sprHdr, 1, sizeof(sprHdr), fp);

	palSize = 256;
	fwrite(&palSize, 1, 2, fp);

	color tempColor;

	unsigned short colorsNum = 0;

	for (i = 0; i < keysNum - 1; i++)
	{
		short rStep = (colorKeys[i + 1].r - colorKeys[i].r) / (colorGradientSize - 1);
		short gStep = (colorKeys[i + 1].g - colorKeys[i].g) / (colorGradientSize - 1);
		short bStep = (colorKeys[i + 1].b - colorKeys[i].b) / (colorGradientSize - 1);

		for (j = (!i ? 0 : 1); j < colorGradientSize - 1; j++)
		{
			for (k = 0; k < colorRowSize; k++)
			{
				tempColor.r = (unsigned char)((colorKeys[i].r + rStep * j) * (koefColors[k] / 256.0));
				tempColor.g = (unsigned char)((colorKeys[i].g + gStep * j) * (koefColors[k] / 256.0));
				tempColor.b = (unsigned char)((colorKeys[i].b + bStep * j) * (koefColors[k] / 256.0));
				fwrite(&tempColor, 1, 3, fp);
				colorsNum++;
			}
		}

		for (k = 0; k < colorRowSize; k++)
		{
			tempColor.r = (unsigned char)(colorKeys[i + 1].r * (koefColors[k] / 256.0));
			tempColor.g = (unsigned char)(colorKeys[i + 1].g * (koefColors[k] / 256.0));
			tempColor.b = (unsigned char)(colorKeys[i + 1].b * (koefColors[k] / 256.0));
			fwrite(&tempColor, 1, 3, fp);
			colorsNum++;
		}
	}

	if (keysNum > 2)
	{
		short rStep = (colorKeys[0].r - colorKeys[keysNum - 1].r) / (colorGradientSize - 1);
		short gStep = (colorKeys[0].g - colorKeys[keysNum - 1].g) / (colorGradientSize - 1);
		short bStep = (colorKeys[0].b - colorKeys[keysNum - 1].b) / (colorGradientSize - 1);

		for (j = 1; j < colorGradientSize - 1; j++)
		{
			if (colorsNum == 255)
				break;

			for (k = 0; k < colorRowSize; k++)
			{
				tempColor.r = (unsigned char)((colorKeys[keysNum - 1].r + rStep * j) * (koefColors[k] / 256.0));
				tempColor.g = (unsigned char)((colorKeys[keysNum - 1].g + gStep * j) * (koefColors[k] / 256.0));
				tempColor.b = (unsigned char)((colorKeys[keysNum - 1].b + bStep * j) * (koefColors[k] / 256.0));
				fwrite(&tempColor, 1, 3, fp);
				colorsNum++;
			}
		}
	}

	tempColor.r = tempColor.g = tempColor.b = 255;
	for (i = colorsNum; i < 256; i++)
		fwrite(&tempColor, 1, 3, fp);
	
	unsigned char tempIndex;

	if (keysNum > 2)
	{
		for (i = 0; i < frameNum; i++)
		{
			fwrite(&frameHdr, 1, sizeof(frameHdr), fp);

			for (j = 0; j < frameHdr.frameWidth * frameHdr.frameHeight; j++)
			{
				if (isIgnoreTransColor && fixIndexes[frame[j]] == 255)
					tempIndex = 255;
				else
					tempIndex = fixIndexes[frame[j]] + i * colorRowSize;
				fwrite(&tempIndex, 1, 1, fp);
			}
		}
	}
	else
	{
		for (i = 0; i < colorGradientSize; i++)
		{
			fwrite(&frameHdr, 1, sizeof(frameHdr), fp);

			for (j = 0; j < frameHdr.frameWidth * frameHdr.frameHeight; j++)
			{
				if (isIgnoreTransColor && fixIndexes[frame[j]] == 255)
					tempIndex = 255;
				else
					tempIndex = fixIndexes[frame[j]] + i * colorRowSize;
				fwrite(&tempIndex, 1, 1, fp);
			}
		}

		for (i = colorGradientSize - 1; i > 0; i--)
		{
			fwrite(&frameHdr, 1, sizeof(frameHdr), fp);

			for (j = 0; j < frameHdr.frameWidth * frameHdr.frameHeight; j++)
			{
				if (isIgnoreTransColor && fixIndexes[frame[j]] == 255)
					tempIndex = 255;
				else
					tempIndex = fixIndexes[frame[j]] + i * colorRowSize;
				fwrite(&tempIndex, 1, 1, fp);
			}
		}
	}

	free(fixIndexes);
	free(koefColors);
	free(frame);

	fclose(fp);
	printf("Success!\n");

	return 0;
}