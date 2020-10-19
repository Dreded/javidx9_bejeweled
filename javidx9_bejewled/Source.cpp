#include <iostream>
#include <string>
#include <algorithm>
#include "olcConsoleGameEngine.h"

#undef min
#undef max


class Gems : public olcConsoleGameEngine
{
public:

	void DrawRect(int x, int y, int nxSize, int nySize, short shade, short colour)
	{
		DrawLine(x, y, x + nxSize, y, shade, colour);					// Top
		DrawLine(x, y + nySize, x + nxSize, y + nySize, shade, colour);	// Bottom
		DrawLine(x + nxSize, y, x + nxSize, y + nySize, shade, colour);	// Right
		DrawLine(x, y, x, y + nySize, shade, colour);					// Left
	}

	Gems()
	{
		m_sAppName = L"Gems Pro!";
	}

	struct sGem
	{
		short colour;
		bool bExist;
		bool bRemove;
		bool bBomb;
	};

	enum STATES
	{
		STATE_USER,
		STATE_SWAP,
		STATE_CHECK,
		STATE_ERASE,
		STATE_COMPRESS,
		STATE_NEWGEMS,
	} nState, nNextState;

	sGem gems[8][8];

	float fDelayTime = 0.0f;
	std::wstring sprGem;
	std::wstring sprBomb;

	int nTotalGems = 0;

	int nCursorX, nCursorY = 0;
	int nSwapX, nSwapY = 0;
	bool bSwapFail = false;
	bool bGemsToRemove = false;


	struct sFragment
	{
		float x;
		float y;
		float vx;
		float vy;
		short colour;
	};

	std::list<sFragment> fragments;

public:
	bool OnUserCreate()
	{
		sprGem += L"..####..";
		sprGem += L".#XXXX#.";
		sprGem += L"#X@@XXX#";
		sprGem += L"#XXXXX@#";
		sprGem += L".#X@@@#.";
		sprGem += L"..#X@#..";
		sprGem += L"...##...";
		sprGem += L"........";

		sprBomb += L"...XX...";
		sprBomb += L".X.XX.X.";
		sprBomb += L"..X@@X..";
		sprBomb += L"XX@@XXXX";
		sprBomb += L"XXXX@@XX";
		sprBomb += L"..X@@X..";
		sprBomb += L".X.XX.X.";
		sprBomb += L"...XX...";

		nState = STATE_USER;
		nNextState = STATE_USER;

		return true;
	}
	bool OnUserUpdate(float fElapsedTime)
	{
		if (fDelayTime > 0.0f)
		{
			fDelayTime -= fElapsedTime;
		}
		else
		{
			auto boom = [&](int x, int y, int size, short colour)
			{
				auto random_float = [&](float min, float max)
				{
					return ((float)rand() / (float)RAND_MAX) * (max - min) + min;
				};
				for (int i = 0; i < size; i++)
				{
					float a = random_float(0, 2.0f * 3.14159f);
					sFragment f = { (float)x, (float)y, cos(a) * random_float(30.0f,75.0f), sin(a) * random_float(30.0f,75.0f), colour };
					fragments.push_back(f);
				}
			};

			// Gameplay

			switch (nState)
			{
			case STATE_USER:
				// Move Cursor
				if (nTotalGems < 64)
					nNextState = STATE_COMPRESS;
				else
				{
					if (!GetKey(VK_SPACE).bHeld)
					{
						if (GetKey('A').bPressed) nCursorX--;
						if (GetKey('D').bPressed) nCursorX++;
						if (GetKey('W').bPressed) nCursorY--;
						if (GetKey('S').bPressed) nCursorY++;

						// Clamp to Screen
						if (nCursorX < 0) nCursorX = 0;
						if (nCursorX > 7) nCursorX = 7;
						if (nCursorY < 0) nCursorY = 0;
						if (nCursorY > 7) nCursorY = 7;
					}
					else
					{
						//Initiate Swap
						nSwapX = nCursorX;
						nSwapY = nCursorY;
						if (GetKey('A').bPressed && nCursorX > 0) nSwapX = nCursorX - 1;
						if (GetKey('D').bPressed && nCursorX < 7) nSwapX = nCursorX + 1;
						if (GetKey('W').bPressed && nCursorY > 0) nSwapY = nCursorY - 1;
						if (GetKey('S').bPressed && nCursorY < 7) nSwapY = nCursorY + 1;
						if (nSwapX != nCursorX || nSwapY != nCursorY) nNextState = STATE_SWAP;
					}
				}
				break;

			case STATE_SWAP:
				bSwapFail = true;
				std::swap(gems[nCursorX][nCursorY], gems[nSwapX][nSwapY]);

				fDelayTime = 0.5f;
				nNextState = STATE_CHECK;
				break;

			case STATE_CHECK:
				bGemsToRemove = false;

				for (int x = 0; x < 8; x++)
					for (int y = 0; y < 8; y++)
					{
						if (!gems[x][y].bRemove)
						{
							bool bPlaceBomb = false;

							// Check Horizontally
							int nChain = 1;
							while (gems[x][y].colour == gems[x + nChain][y].colour && (nChain + x) < 8) nChain++;
							if (nChain >= 3)
							{
								if (nChain >= 4) bPlaceBomb = true;
								while (nChain > 0)
								{
									gems[x + nChain - 1][y].bRemove = true;

									if (gems[x + nChain - 1][y].bBomb)
									{
										for (int i = -1; i < 2; i++)
											for (int j = -1; j < 2; j++)
											{
												int m = std::min(std::max(i + (x + nChain - 1), 0), 7);
												int n = std::min(std::max(j + y, 0), 7);
												gems[m][n].bRemove = true;
											}
									}
									nChain--;
									bSwapFail = false;
									bGemsToRemove = true;
								}
							}

							// Check Vertically
							nChain = 1;
							while (gems[x][y].colour == gems[x][y + nChain].colour && (nChain + y) < 8) nChain++;
							if (nChain >= 3)
							{
								if (nChain >= 4) bPlaceBomb = true;
								while (nChain > 0)
								{
									gems[x][y + nChain - 1].bRemove = true;

									if (gems[x][y + nChain - 1].bBomb)
									{
										for (int i = -1; i < 2; i++)
											for (int j = -1; j < 2; j++)
											{
												int m = std::min(std::max(i + x, 0), 7);
												int n = std::min(std::max(j + (y + nChain - 1), 0), 7);
												gems[m][n].bRemove = true;
											}
									}
									nChain--;
									bSwapFail = false;
									bGemsToRemove = true;
								}
							}
							if (bPlaceBomb)
							{
								gems[x][y].bBomb = true;
								gems[x][y].bRemove = false;
							}
						}
					}
				if (bGemsToRemove)
					fDelayTime = 0.5f;

				nNextState = STATE_ERASE;
				break;

			case STATE_ERASE:
				if (!bGemsToRemove)
				{
					if (bSwapFail)
						std::swap(gems[nCursorX][nCursorY], gems[nSwapX][nSwapY]);
					nNextState = STATE_USER;
				}
				else
				{
					for (int x = 0; x < 8; x++)
						for (int y = 0; y < 8; y++)
							if (gems[x][y].bRemove)
							{
								gems[x][y].bExist = false;
								gems[x][y].bBomb = false;
								boom((x * 8)+4, (y * 8)+4, 50, gems[x][y].colour);
								nTotalGems--;
							}

					nNextState = STATE_COMPRESS;
				}
				break;

			case STATE_COMPRESS:
				for (int y = 6; y >= 0; y--)
					for (int x = 0; x < 8; x++)
						if (gems[x][y].bExist && !gems[x][y + 1].bExist)
							std::swap(gems[x][y], gems[x][y + 1]);

				fDelayTime = 0.1f;
				nNextState = STATE_NEWGEMS;
				break;

			case STATE_NEWGEMS:
				for (int x = 0; x < 8; x++)
					if (!gems[x][0].bExist)
					{
						gems[x][0].bExist = true;
						gems[x][0].bRemove = false;
						gems[x][0].bBomb = false;
						gems[x][0].colour = rand() % 7 + 8;
						nTotalGems++;
					}

				if (nTotalGems < 64)
				{
					fDelayTime = 0.1f;
					nNextState = STATE_COMPRESS;
				}
				else
					nNextState = STATE_CHECK;
				break;

			}

			nState = nNextState;

		} // End of GamePlay

		// Rendering
		Fill(0, 0, ScreenWidth(), ScreenHeight(), FG_BLACK);

		// Draw Field
		for (int x = 0; x < 8; x++)
			for (int y = 0; y < 8; y++)
				if (gems[x][y].bExist)
					for (int i = 0; i < 8; i++)
						for (int j = 0; j < 8; j++)
						{
							std::wstring& source = gems[x][y].bBomb ? sprBomb : sprGem;
							if (source[j * 8 + i] != L'.')
							{
								short colour = gems[x][y].colour;
								short shading = PIXEL_SOLID;
								if (source[j * 8 + i] == L'#') shading = PIXEL_HALF;
								if (source[j * 8 + i] == L'@') colour -= 8;
								if (gems[x][y].bRemove) shading = PIXEL_QUARTER;

								Draw(x * 8 + i, y * 8 + j,  shading, colour);
							}
						}


		// Draw Cursor
		DrawRect(nCursorX * 8, nCursorY * 8, 7, 7, PIXEL_SOLID, FG_WHITE);

		for (auto& f : fragments)
		{
			Draw(f.x, f.y, PIXEL_SOLID, f.colour);

			f.x += f.vx * fElapsedTime;
			f.y += f.vy * fElapsedTime;
		}

		std::remove_if(fragments.begin(), fragments.end(),
			[&](const sFragment& f)
			{
				return f.x<0 || f.x>ScreenWidth() || f.y<0 || f.y>ScreenHeight();
			});

		return true;
	}
};


int main()
{
	Gems game;
	if (game.ConstructConsole(64, 64, 14, 14))
		game.Start();

	return 0;
}