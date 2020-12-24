/*
	Copyright (c) 2020 Adrian Siekierka

	Based on a reconstruction of code from ZZT,
	Copyright 1991 Epic MegaGames, used with permission.

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

/*$I-*/
/*$V-*/

#include "ptoc.h"
#include "serialization.h"

#include <dirent.h>
#include <iterator>

#define __Game_implementation__


#include "game.h"
#include "tools.h"

#include "dos.h"
/*#include "Crt.h"*/
#include "video.h"
#include "sounds.h"
#include "elements.h"
//#include "editor.h"
#include "oop.h"
#include "fileops.h"
#include "minmax.h"

#include "hardware.h"

boolean ValidCoord(integer x, integer y) {
	boolean ValidCoord_result;
	if ((x < 0) || (y < 0))  {
		ValidCoord_result = false;
		return ValidCoord_result;
	}
	if ((x > BOARD_WIDTH+1) || (y > BOARD_HEIGHT+1))  {
		ValidCoord_result = false;
		return ValidCoord_result;
	}
	ValidCoord_result = true;
	return ValidCoord_result;
}

boolean CoordInsideViewport(integer x, integer y) {
	boolean CoordInsideViewport_result;
	if ((x <= 0) || (y <= 0))  {
		CoordInsideViewport_result = false;
		return CoordInsideViewport_result;
	}
	if ((x > BOARD_WIDTH) || (y > BOARD_HEIGHT))  {
		CoordInsideViewport_result = false;
		return CoordInsideViewport_result;
	}
	CoordInsideViewport_result = true;
	return CoordInsideViewport_result;
}

void SidebarClearLine(integer y) {
	video.VideoWriteText(60, y, 0x11, "\263                   ");
}

void SidebarClear() {
	integer i;

	for( i = 3; i <= 24; i ++)
		SidebarClearLine(i);
}

void GenerateTransitionTable() {
	integer ix, iy;
	TCoord t;

	TransitionTableSize = 0;
	for( iy = 1; iy <= BOARD_HEIGHT; iy ++)
		for( ix = 1; ix <= BOARD_WIDTH; ix ++) {
			TransitionTableSize = TransitionTableSize + 1;
			TransitionTable[TransitionTableSize].X = ix;
			TransitionTable[TransitionTableSize].Y = iy;
		}

	/* shuffle */
	for( ix = 1; ix <= TransitionTableSize; ix ++) {
		iy = Random(TransitionTableSize) + 1;
		t = TransitionTable[iy];
		TransitionTable[iy] = TransitionTable[ix];
		TransitionTable[ix] = t;
	}
}

// Move from a structure to another pointed at by a pointer.
template<typename T, typename Q> void Move(T & structureOne, Q & ptrTwo,
        size_t length) {
	bcopy(&structureOne, ptrTwo, length);
}

template<typename T, typename Q> void MoveP(T & structureOne, Q & structureTwo,
        size_t length) {
    bcopy(&structureOne, &structureTwo, length);
}

void BoardClose(boolean showTruncationNote) {
	World.BoardData[World.Info.CurrentBoard] = Board.dump();
	World.BoardLen[World.Info.CurrentBoard] =
		World.BoardData[World.Info.CurrentBoard].size();
}

/* Set worldIsDamaged to true if the BoardOpen is from a world load and
the world metadata is wrong; this will make the corruption notification
show up regardless of whether the board itself is damaged. */
void BoardOpen(integer boardId, boolean worldIsDamaged) {

	byte* ptr;
	integer i, ix, iy;
	TRleTile rle;
	integer bytesRead;
	boolean boardIsDamaged;

	if (boardId > World.BoardCount)
		boardId = World.Info.CurrentBoard;

	std::string load_error = Board.load(World.BoardData[boardId],
		boardId, World.BoardCount);

	World.Info.CurrentBoard = boardId;

	// TODO: Distinguish between truncation and corruption as in
	// FPC. Also be more sensible about what kind of corruption
	// notices to show -- probably just one at the very end.
	if (load_error != "" || worldIsDamaged) {
		DisplayCorruptionNote(load_error);
	}

}

void BoardChange(integer boardId) {
	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element = E_PLAYER;
	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Color =
	    ElementDefs[E_PLAYER].Color;
	if (boardId != World.Info.CurrentBoard)  {
		BoardClose(true);
		BoardOpen(boardId, false);
	}
}

void BoardCreate() {
	Board.create();
}

void WorldCreate() {
	integer i;

	InitElementsGame();
	World.BoardCount = 0;
	World.BoardLen[0] = 0;
	InitEditorStatSettings();
	ResetMessageNotShownFlags();
	BoardCreate();
	World.Info.IsSave = false;
	World.Info.CurrentBoard = 0;
	World.Info.Ammo = 0;
	World.Info.Gems = 0;
	World.Info.Health = 100;
	World.Info.EnergizerTicks = 0;
	World.Info.Torches = 0;
	World.Info.TorchTicks = 0;
	World.Info.Score = 0;
	World.Info.BoardTimeSec = 0;
	World.Info.BoardTimeHsec = 0;
	for( i = 1; i <= 7; i ++) {
		World.Info.TakeKey(i);
	}
	for( i = 1; i <= 10; i ++)
		World.Info.Flags[i] = "";
	BoardChange(0);
	Board.Name = "Title screen";
	LoadedGameFileName = "";
	World.Info.Name = "";
}

void TransitionDrawToFill(char chr_, integer color) {
	integer i;

	for( i = 1; i <= TransitionTableSize; i ++)
		video.VideoWriteText(TransitionTable[i].X - 1, TransitionTable[i].Y - 1,
		                     color,
		                     chr_);
}

void BoardDrawTile(integer x, integer y) {
	byte ch;

	{
		TTile& with = Board.Tiles[x][y];
		if (! Board.Info.IsDark
		        || (ElementDefs[Board.Tiles[x][y].Element].VisibleInDark)
		        || (
		            (World.Info.TorchTicks > 0)
		            && ((sqr(Board.Stats[0].X - x) + sqr(Board.Stats[0].Y - y) * 2) <
		                TORCH_DIST_SQR)
		        ) || ForceDarknessOff) {
			if (with.Element == E_EMPTY)
				video.VideoWriteText(x - 1, y - 1, 0xf, " ");
			else if ((with.Element < E_TEXT_MIN)
			         && ElementDefs[with.Element].HasDrawProc)  {
				ElementDefs[with.Element].DrawProc(x, y, ch);
				video.VideoWriteText(x - 1, y - 1, with.Color, chr(ch));
			} else if (with.Element < E_TEXT_MIN)
				video.VideoWriteText(x - 1, y - 1, with.Color,
				                     ElementDefs[with.Element].Character);
			else {
				/* Text drawing */
				if (with.Element == E_TEXT_WHITE)
					video.VideoWriteText(x - 1, y - 1, 0xf, chr(Board.Tiles[x][y].Color));
				else if (VideoMonochrome)
					video.VideoWriteText(x - 1, y - 1, ((with.Element - E_TEXT_MIN) + 1) * 16,
					                     chr(Board.Tiles[x][y].Color));
				else
					video.VideoWriteText(x - 1, y - 1,
					                     (((with.Element - E_TEXT_MIN) + 1) * 16) + 0xf,
					                     chr(Board.Tiles[x][y].Color));
			}
		} else {
			/* Darkness */
			video.VideoWriteText(x - 1, y - 1, 0x7, "\260");
		}
	}
}

void BoardDrawBorder() {
	integer ix, iy;

	for( ix = 1; ix <= BOARD_WIDTH; ix ++) {
		BoardDrawTile(ix, 1);
		BoardDrawTile(ix, BOARD_HEIGHT);
	}

	for( iy = 1; iy <= BOARD_HEIGHT; iy ++) {
		BoardDrawTile(1, iy);
		BoardDrawTile(BOARD_WIDTH, iy);
	}
}

void TransitionDrawToBoard() {
	integer i;

	BoardDrawBorder();

	for( i = 1; i <= TransitionTableSize; i ++) {
		TCoord& with = TransitionTable[i];
		BoardDrawTile(with.X, with.Y);
	}
}

void SidebarPromptCharacter(boolean editable, integer x, integer y,
                            TString50 prompt, byte& value) {
	integer i, newValue;

	SidebarClearLine(y);
	video.VideoWriteText(x, y, (integer)(editable) + 0x1e, prompt);
	SidebarClearLine(y + 1);
	video.VideoWriteText(x + 5, y + 1, 0x9f, "\37");
	SidebarClearLine(y + 2);

	do {
		for( i = (value - 4); i <= (value + 4); i ++)
			video.VideoWriteText(((x + i) - value) + 5, y + 2, 0x1e,
			                     chr((i + 0x100) % 0x100));

		if (editable)  {
			// All of these can be replaced with a blocking read later.
			// It'll only make things more responsive.
			Delay(25);
			InputUpdate();
			if (InputKeyPressed == E_KEY_TAB)
				InputDeltaX = 9;

			newValue = value + InputDeltaX;
			if (value != newValue)  {
				value = (newValue + 0x100) % 0x100;
				SidebarClearLine(y + 2);
			}
		}
	} while (!((InputKeyPressed == E_KEY_ENTER)
	           || (InputKeyPressed == E_KEY_ESCAPE) || ! editable || InputShiftPressed));

	video.VideoWriteText(x + 5, y + 1, 0x1f, "\37");
}

void SidebarPromptSlider(boolean editable, integer x, integer y,
                         string prompt, byte& value) {
	integer newValue;
	char startChar, endChar;

	if (prompt[length(prompt) - 2] == ';')  {
		startChar = prompt[length(prompt) - 1];
		endChar = prompt[length(prompt)];
		prompt = copy(prompt, 1, length(prompt) - 3);
	} else {
		startChar = '1';
		endChar = '9';
	}

	SidebarClearLine(y);
	video.VideoWriteText(x, y, (integer)(editable) + 0x1e, prompt);
	SidebarClearLine(y + 1);
	SidebarClearLine(y + 2);
	video.VideoWriteText(x, y + 2, 0x1e,
	                     string(startChar) + "....:...." + endChar);

	do {
		if (editable)  {
			if (InputJoystickMoved)
				Delay(45);
			video.VideoWriteText(x + value + 1, y + 1, 0x9f, "\37");

			InputUpdate();
			if ((InputKeyPressed >= '1') && (InputKeyPressed <= '9'))  {
				value = ord(InputKeyPressed) - 49;
				SidebarClearLine(y + 1);
			} else {
				newValue = value + InputDeltaX;
				if ((value != newValue) && (newValue >= 0) && (newValue <= 8))  {
					value = newValue;
					SidebarClearLine(y + 1);
				}
			}
		}
	} while (!((InputKeyPressed == E_KEY_ENTER)
	           || (InputKeyPressed == E_KEY_ESCAPE) || ! editable || InputShiftPressed));

	video.VideoWriteText(x + value + 1, y + 1, 0x1f, "\37");
}

void SidebarPromptChoice(boolean editable, integer y, string prompt,
                         string choiceStr, byte& result) {
	integer i, j, choiceCount;
	integer newResult;

	SidebarClearLine(y);
	SidebarClearLine(y + 1);
	SidebarClearLine(y + 2);
	video.VideoWriteText(63, y, (integer)(editable) + 0x1e, prompt);
	video.VideoWriteText(63, y + 2, 0x1e, choiceStr);

	choiceCount = 1;
	for( i = 1; i <= length(choiceStr); i ++)
		if (choiceStr[i] == ' ')
			choiceCount = choiceCount + 1;

	do {
		j = 0;
		i = 1;
		while ((j < result) && (i < length(choiceStr)))  {
			if (choiceStr[i] == ' ')
				j = j + 1;
			i = i + 1;
		}

		if (editable)  {
			video.VideoWriteText(62 + i, y + 1, 0x9f, "\37");
			Delay(35);
			InputUpdate();

			newResult = result + InputDeltaX;
			if ((result != newResult) && (newResult >= 0)
			        && (newResult <= (choiceCount - 1)))  {
				result = newResult;
				SidebarClearLine(y + 1);
			}
		}
	} while (!((InputKeyPressed == E_KEY_ENTER)
	           || (InputKeyPressed == E_KEY_ESCAPE) || ! editable || InputShiftPressed));

	video.VideoWriteText(62 + i, y + 1, 0x1f, "\37");
}

void SidebarPromptDirection(boolean editable, integer y, string prompt,
                            integer& deltaX, integer& deltaY) {
	byte choice;

	if (deltaY == -1)
		choice = 0;
	else if (deltaY == 1)
		choice = 1;
	else if (deltaX == -1)
		choice = 2;
	else
		choice = 3;
	SidebarPromptChoice(editable, y, prompt, "\30 \31 \33 \32", choice);
	deltaX = NeighborDeltaX[choice];
	deltaY = NeighborDeltaY[choice];
}

void PromptString(integer x, integer y, integer arrowColor, integer color,
                  integer width, byte mode, TString50& buffer) {
	integer i;
	string oldBuffer;
	boolean firstKeyPress;

	oldBuffer = buffer;
	firstKeyPress = true;

	do {
		for( i = 0; i <= (width - 1); i ++) {
			video.VideoWriteText(x + i, y, color, " ");
			video.VideoWriteText(x + i, y - 1, arrowColor, " ");
		}
		video.VideoWriteText(x + width, y - 1, arrowColor, " ");
		video.VideoWriteText(x + length(buffer), y - 1,
		                     (arrowColor / 0x10) * 16 + 0xf,
		                     "\37");
		video.VideoWriteText(x, y, color, buffer);

		InputReadWaitKey();

		if ((length(buffer) < width) && (InputKeyPressed >= '\40')
		        && (! InputSpecialKeyPressed))  {
			if (firstKeyPress)
				buffer = "";
			switch (mode) {
			case PROMPT_NUMERIC: {
				if (set::of(range('0', '9'), eos).has(InputKeyPressed))  {
					buffer = buffer + (char)InputKeyPressed;
				}
			}
			break;
			case PROMPT_ANY: {
				buffer = buffer + (char)InputKeyPressed;
			}
			break;
			case PROMPT_ALPHANUM: {
				if ((set::of(range('A', 'Z'), eos).has(keyUpCase(InputKeyPressed)))
				        || (set::of(range('0', '9'), eos).has(InputKeyPressed))
				        || (InputKeyPressed == '-')) {
					buffer = buffer + (char)keyUpCase(InputKeyPressed);
				}
			}
			break;
			}
		} else if ((InputKeyPressed == E_KEY_LEFT)
		           || (InputKeyPressed == E_KEY_BACKSPACE))  {
			buffer = copy(buffer, 1, length(buffer) - 1);
			/*IMP: Clear the whole line if Home is pressed.*/
		} else if (InputKeyPressed == E_KEY_HOME)  {
			buffer = "";
		}

		firstKeyPress = false;
	} while (!((InputKeyPressed == E_KEY_ENTER)
	           || (InputKeyPressed == E_KEY_ESCAPE)));
	if (InputKeyPressed == E_KEY_ESCAPE)  {
		buffer = oldBuffer;
	}
}

boolean SidebarPromptYesNo(string message, boolean defaultReturn) {
	boolean SidebarPromptYesNo_result;
	SidebarClearLine(3);
	SidebarClearLine(4);
	SidebarClearLine(5);
	video.VideoWriteText(63, 5, 0x1f, message);
	video.VideoWriteText(63 + length(message), 5, 0x9e, "_");

	do {
		InputReadWaitKey();
	} while (!(set::of(E_KEY_ESCAPE, 'N', 'Y',
	                   eos).has(keyUpCase(InputKeyPressed))));
	if (keyUpCase(InputKeyPressed) == 'Y')
		defaultReturn = true;
	else
		defaultReturn = false;

	SidebarClearLine(5);
	SidebarPromptYesNo_result = defaultReturn;
	return SidebarPromptYesNo_result;
}

void SidebarPromptString(string prompt, TString50 extension,
                         TString50& filename, byte promptMode) {
	SidebarClearLine(3);
	SidebarClearLine(4);
	SidebarClearLine(5);
	video.VideoWriteText(75 - length(prompt), 3, 0x1f, prompt);
	video.VideoWriteText(63, 5, 0xf, string("        ") + extension);

	PromptString(63, 5, 0x1e, 0xf, 8, promptMode, filename);

	SidebarClearLine(3);
	SidebarClearLine(4);
	SidebarClearLine(5);
}

void PauseOnError() {
	SoundQueue(1, SoundParse("s004x114x9"));
	Delay(2000);
}

boolean DisplayIOError() {
	varying_string<50> errorNumStr;
	TTextWindowState textWindow;

	if (errno == 0)  {
		return false;		// no error
	}

	/*IMP: Use explanations instead of numeric error codes. */
	textWindow.Title = string("Error: ") + strerror(errno);
	TextWindowInitState(textWindow);
	TextWindowAppend(textWindow, "$DOS Error: ");
	TextWindowAppend(textWindow, "");
	TextWindowAppend(textWindow, "This may be caused by missing");
	TextWindowAppend(textWindow, "ZZT files or a bad disk.  If");
	TextWindowAppend(textWindow, "you are trying to save a game,");
	TextWindowAppend(textWindow, "your disk may be full -- try");
	TextWindowAppend(textWindow, "using a blank, formatted disk");
	TextWindowAppend(textWindow, "for saving the game!");

	TextWindowDrawOpen(textWindow);
	TextWindowSelect(textWindow, false, false);
	TextWindowDrawClose(textWindow);
	TextWindowFree(textWindow);

	// So we don't trigger twice on the same error.
	errno = 0;

	return true;
}

void DisplayTruncationNote() {
	TTextWindowState textWindow;

	textWindow.Title = "Warning: Potential data loss";
	TextWindowInitState(textWindow);
	TextWindowAppend(textWindow, "$Warning:");
	TextWindowAppend(textWindow, "");
	TextWindowAppend(textWindow, "A board that was just saved was too large");
	TextWindowAppend(textWindow, "and some data had to be cut. This might");
	TextWindowAppend(textWindow, "lead to data loss. If you haven't saved");
	TextWindowAppend(textWindow, "yet, do so under another name and make");
	TextWindowAppend(textWindow, "the board smaller!");
	TextWindowAppend(textWindow, "");
	TextWindowAppend(textWindow, "If you're just playing, tell the author");
	TextWindowAppend(textWindow, "of the world that you're playing.");

	TextWindowDrawOpen(textWindow);
	TextWindowSelect(textWindow, false, false);
	TextWindowDrawClose(textWindow);
	TextWindowFree(textWindow);
}

void DisplayCorruptionNote(std::string corruption_type) {
	TTextWindowState textWindow;

	textWindow.Title = "Warning: Corruption detected";
	TextWindowInitState(textWindow);
	TextWindowAppend(textWindow, "$Warning:");
	TextWindowAppend(textWindow, "");
	TextWindowAppend(textWindow, "The file or board that was just loaded");
	TextWindowAppend(textWindow, "contained some damaged information.");
	TextWindowAppend(textWindow, "This might be caused by a bad file");
	TextWindowAppend(textWindow, "or disk corruption. ZZT has tried");
	TextWindowAppend(textWindow, "to undo the damage, but some data");
	TextWindowAppend(textWindow, "might be lost.");

	if (corruption_type != "") {
		TextWindowAppend(textWindow, "");
		TextWindowAppend(textWindow, "This was corrupted:");
		TextWindowAppend(textWindow, (" - " + corruption_type + ".").c_str());
	}

	TextWindowDrawOpen(textWindow);
	TextWindowSelect(textWindow, false, false);
	TextWindowDrawClose(textWindow);
	TextWindowFree(textWindow);
}

void WorldUnload() {
	integer i;

	/* no need to show any notices if the world's to be unloaded. */
	BoardClose(false);
	/* Reallocating to 0 is better than freeing because it's then
	possible to reallocate back to a higher level later, if
	required. That BoardLen is 0 also makes it obvious that the
	boards have been unloaded. */
	for( i = 0; i <= World.BoardCount; i ++) {
		World.BoardLen[i] = 0;
		//ReAllocMem(World.BoardData[i], World.BoardLen[i]);
	}
}

boolean WorldLoad(std::string filename, std::string extension);

static integer loadProgress;

static void SidebarAnimateLoading() {
	video.VideoWriteText(69, 5, ProgressAnimColors[loadProgress],
	                     ProgressAnimStrings[loadProgress]);
	loadProgress = (loadProgress + 1) % 8;
}

boolean WorldLoad(std::string filename, std::string extension) {

	std::vector<unsigned char>::const_iterator ptr;
	integer boardId;
	word actuallyRead;
	integer firstZero;
	integer i;
	boolean worldIsDamaged;

	boolean WorldLoad_result;
	WorldLoad_result = false;
	worldIsDamaged = false;
	loadProgress = 0;

	SidebarClearLine(4);
	SidebarClearLine(5);
	SidebarClearLine(5);
	video.VideoWriteText(62, 5, 0x1f, "Loading.....");

	if (filename + extension == "")  return WorldLoad_result;

	std::string full_filename = std::string(filename + extension);
	std::ifstream f = OpenForRead(full_filename);

	if (! DisplayIOError())  {
		// Unload the current error. TODO: Do RAII-style.
		WorldUnload();

		// Buffer.
		std::vector<unsigned char> world_header(512);

		f.read((char *)world_header.data(), 512);
		ptr = world_header.begin();
		if (! DisplayIOError())  {
			ptr = load_lsb_element(ptr, World.BoardCount);

			// If the file starts FF, then the board count is next.
			// Otherwise, if below zero, that indicates another version
			// of ZZT. Otherwise (positive count), that's the board count.
			if (World.BoardCount < 0)  {
				if (World.BoardCount != -1)  {
					video.VideoWriteText(63, 5, 0x1e, "You need a newer");
					video.VideoWriteText(63, 6, 0x1e, " version of ZZT!");
					return WorldLoad_result;
				} else {
					ptr = load_lsb_element(ptr, World.BoardCount);
				}
			}

			// TODO: I know these aren't vector pointers. fix later.
			ptr = World.Info.load(ptr, world_header.end());

			/* If the board count is negative, set it to zero. This should
			also signal that the world is corrupt. Another option would be
			to make all the fields unsigned, but who needs worlds with more
			than 32 thousand boards anyway? Besides, they'd crash DOS ZZT. */
			if (World.BoardCount < 0)  {
				World.BoardCount = 0;
				worldIsDamaged = true;
			}

			/* If there are too many boards, ditto. (That's a more serious
			problem, as it may cut off boards outright.) */
			if (World.BoardCount > MAX_BOARD)  {
				World.BoardCount = MAX_BOARD;
				worldIsDamaged = true;
			}

			/* Don't accept CurrentBoard values that are too large or
			small. */
			if (World.Info.CurrentBoard > World.BoardCount ||
				World.Info.CurrentBoard < 0)  {

				World.Info.CurrentBoard = Max(0, Min(World.BoardCount,
				                                     World.Info.CurrentBoard));
				worldIsDamaged = true;
			}

			for( boardId = 0; boardId <= World.BoardCount; boardId ++) {
				SidebarAnimateLoading();

				if (boardId > World.BoardCount)  continue;

                // XXX: Problem with endian assumptions and assumption
                // that the output is 16 bits.
                char thisBoardLenBuf[2];
                unsigned short thisBoardLen = 0;
                f.read(thisBoardLenBuf, 2);
                load_lsb_element(thisBoardLenBuf, thisBoardLen);

                World.BoardLen[boardId] = thisBoardLen;

				/* Sanity check. Abort at this position so that any
				boards before the corrupted one can still be
							  recovered.*/
				if ((DisplayIOError()) || (World.BoardLen[boardId] < 0))  {
					World.BoardLen[boardId] = 0;
					worldIsDamaged = true;
					if (boardId == 0)  {
						WorldUnload();
						return WorldLoad_result;
					}
					World.BoardCount = boardId - 1;
					/* No more boards to be had, so break. */
					break;
				} else {
					/* If it's the last board, get everything we can.
						This recovers the last Super Lock-corrupted board.
						actuallyRead below will adjust the board length back
						if we're dealing with an ordinary world. */
					if (boardId == World.BoardCount)
						World.BoardLen[boardId] = MAX_BOARD_LEN;
					World.BoardData[boardId].resize(World.BoardLen[boardId]);
					f.read((char *)World.BoardData[boardId].data(),
						World.BoardLen[boardId]);
					actuallyRead = f.gcount();

					/* SANITY: If reading the whole board would lead to an
						overflow down the line, pretend we only read the
						MAX_BOARD_LEN first. */
					if (actuallyRead > MAX_BOARD_LEN)  {
						actuallyRead = Min(actuallyRead, MAX_BOARD_LEN);
						worldIsDamaged = true;
					}

					World.BoardData[boardId].resize(actuallyRead);

					/* SANITY: reallocate and update board len if
						there's a mismatch between how much we were told
						we could read, and how much we actually read.
						This also cuts down very large boards that we can't
						represent in memory anyway (size > 20k). */
					if (actuallyRead != World.BoardLen[boardId])  {
						World.BoardLen[boardId] = actuallyRead;
					}
				}
			}

			f.close();

			/* More sanity checks. If the current board number is negative
				or too high, set it to zero. (Maybe instead set it to the
				actual number of boards read?) */
			if ((World.Info.CurrentBoard < 0) ||
				(World.Info.CurrentBoard > Min(MAX_BOARD, World.BoardCount)))
				World.Info.CurrentBoard = 0;

			BoardOpen(World.Info.CurrentBoard, worldIsDamaged);
			LoadedGameFileName = filename.c_str();

            // XXX: Once we add in editor.cxx
			//HighScoresLoad();

			SidebarClearLine(5);
		}
	}
	return true;
}

void WorldSave(TString50 filename, TString50 extension) {
	untyped_file f;
	integer i;
	integer unk1;
	TIoTmpBuf* ptr;
	integer version;


	BoardClose(true);
	video.VideoWriteText(63, 5, 0x1f, "Saving...");

	std::string full_filename = std::string(filename + extension);
	std::ofstream out_file = OpenForWrite(full_filename);

    // TODO IMP? Perhaps write to a temporary filename and then move it over
    // the original to create some kind of atomicity?
    // https://en.cppreference.com/w/cpp/io/c/tmpnam etc.

	if (! DisplayIOError())  {
		std::vector<unsigned char> world_header;
		append_lsb_element((short)-1, world_header); // Version
		append_lsb_element(World.BoardCount, world_header);
		World.Info.dump(world_header);
		// Pad to 512
		append_zeroes(512-world_header.size(), world_header);

		word actually_written;
		out_file.write((const char *)world_header.data(), 512);

		if (DisplayIOError())  goto LOnError;

		for( i = 0; i <= World.BoardCount; i ++) {
            unsigned short board_len = World.BoardLen[i];
            out_file.write((char *)&board_len, 2);
			if (DisplayIOError())  goto LOnError;

			out_file.write((const char *)World.BoardData[i].data(),
				World.BoardData[i].size());

			if (DisplayIOError())  goto LOnError;
		}

		out_file.close();
	}

	BoardOpen(World.Info.CurrentBoard, false);
	SidebarClearLine(5);
	return;

LOnError:
	out_file.close();
	std::remove(full_filename.c_str()); // Delete the corrupted file.
	// IMP? Give error message? But the above already does.
    BoardOpen(World.Info.CurrentBoard, false);
	SidebarClearLine(5);
}

void GameWorldSave(TString50 prompt, TString50& filename,
                   TString50 extension) {
	TString50 newFilename;

	newFilename = filename;
	SidebarPromptString(prompt, extension, newFilename, PROMPT_ALPHANUM);
	if ((InputKeyPressed != E_KEY_ESCAPE) && (length(newFilename) != 0))  {
		filename = newFilename;
		if (extension == ".ZZT")
			World.Info.Name = filename;
		WorldSave(filename, extension);
	}
}

boolean GameWorldLoad(TString50 extension) {
	TTextWindowState textWindow;
	SearchRec fileSearchRec;
	string entryName;
	integer i;

	boolean GameWorldLoad_result;
	TextWindowInitState(textWindow);
	if (extension == ".ZZT")
		textWindow.Title = "ZZT Worlds";
	else
		textWindow.Title = "Saved Games";
	GameWorldLoad_result = false;
	textWindow.Selectable = true;

    // https://stackoverflow.com/questions/612097

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (".")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
        	std::string filename = ent->d_name;
			// If it doesn't have the right extension, skip.
			// TODO: Make case-insensitive. But the way extensions are
			// handled currently makes that a pain.
        	if (filename.find(extension) == std::string::npos)
        		continue;

        	// Strip the extension
        	filename = filename.substr(0, filename.find(extension));
            TextWindowAppend(textWindow, ent->d_name);
        }
        closedir (dir);
    }

	TextWindowSort(textWindow); /* Sort the file names. */
	TextWindowAppend(textWindow, "Exit");

	TextWindowDrawOpen(textWindow);
	TextWindowSelect(textWindow, false, false);
	TextWindowDrawClose(textWindow);

	if ((textWindow.LinePos < textWindow.LineCount) && ! TextWindowRejected)  {
		entryName = *textWindow.Lines[textWindow.LinePos];
		if (pos(" ", entryName) != 0)
			entryName = copy(entryName, 1, pos(" ", entryName) - 1);

		GameWorldLoad_result = WorldLoad(std::string(entryName),
			std::string(extension));
		TransitionDrawToFill('\333', 0x44);
	}

	TextWindowFree(textWindow);
	return GameWorldLoad_result;
}

void CopyStatDataToTextWindow(integer statId, TTextWindowState& state) {
	string dataStr;
	char dataChr;
	integer i;

	TStat& with = Board.Stats[statId];
	TextWindowInitState(state);
	dataStr = "";

	/* IMP: Fix off-by-one: Don't start counting
	from 0 when copying data. */
	for( i = 0; i < with.DataLen; i ++) {
		dataChr = with.data.get()[i];
		if (dataChr == E_KEY_ENTER)  {
			TextWindowAppend(state, dataStr);
			dataStr = "";
		} else {
			dataStr = dataStr + dataChr;
		}
	}
}

void AddStat(integer tx, integer ty, byte element, integer color,
             integer tcycle, TStat template_) {
	/* First of all: check if we have space. If not, no can do! */
	if (World.BoardLen[World.Info.CurrentBoard] +
		template_.packed_size() > MAX_BOARD_LEN) {
		return;
	}

	/* Can't put anything on top of the player. */
	if ((tx == Board.Stats[0].X) && (ty == Board.Stats[0].Y)) { return; }

	// Can't make more than MAX_STAT stats.
	if (Board.StatCount >= MAX_STAT) { return; }

	Board.StatCount = Board.StatCount + 1;
	Board.Stats[Board.StatCount] = template_;

	// Beware: this counts the data as well. In our case, that's what we
	// *want* because we'll do a deep copy, but that's not always the
	// case! Note to self if/when I change this.
	World.BoardLen[World.Info.CurrentBoard] =
		World.BoardLen[World.Info.CurrentBoard] + template_.packed_size();

	TStat new_stat;
	new_stat.X = tx;
	new_stat.Y = ty;
	new_stat.Cycle = tcycle;
	new_stat.Under = Board.Tiles[tx][ty];
	new_stat.DataPos = 0;

	// AddStat always does a deep copy.
	if (template_.DataLen > 0) {
		new_stat.data = std::shared_ptr<unsigned char[]>(
			new unsigned char[template_.DataLen]);

		std::copy(template_.data.get(),
			template_.data.get() + template_.DataLen,
			new_stat.data.get());
	}

	if (ElementDefs[Board.Tiles[tx][ty].Element].PlaceableOnTop)
		Board.Tiles[tx][ty].Color = (color & 0xf) + (Board.Tiles[tx][ty].Color &
		                            0x70);
	else
		Board.Tiles[tx][ty].Color = color;
	Board.Tiles[tx][ty].Element = element;

	if (CoordInsideViewport(tx, ty)) {
		BoardDrawTile(tx, ty);
	}
}

void RemoveStat(integer statId) {
	integer i;

	// TODO: Update board size.

	Board.Stats[statId].data = NULL;	// deallocates if necessary

	TStat& with = Board.Stats[statId];

	if (statId < CurrentStatTicked)
		CurrentStatTicked = CurrentStatTicked - 1;

	Board.Tiles[with.X][with.Y] = with.Under;
	if (with.Y > 0)
		BoardDrawTile(with.X, with.Y);

	for( i = 1; i <= Board.StatCount; i ++) {
		if (Board.Stats[i].Follower >= statId)  {
			if (Board.Stats[i].Follower == statId)
				Board.Stats[i].Follower = -1;
			else
				Board.Stats[i].Follower = Board.Stats[i].Follower - 1;
		}

		if (Board.Stats[i].Leader >= statId)  {
			if (Board.Stats[i].Leader == statId)
				Board.Stats[i].Leader = -1;
			else
				Board.Stats[i].Leader = Board.Stats[i].Leader - 1;
		}
	}

	for( i = (statId + 1); i <= Board.StatCount; i ++)
		Board.Stats[i - 1] = Board.Stats[i];
	Board.StatCount = Board.StatCount - 1;
}

integer GetStatIdAt(integer x, integer y) {
	integer i;

	i = -1;
	do {
		i = i + 1;
	} while (!(((Board.Stats[i].X == x) && (Board.Stats[i].Y == y))
	           || (i > Board.StatCount)));

	if (i > Board.StatCount)
		return -1;
	else
		return i;
}

boolean BoardPrepareTileForPlacement(integer x, integer y) {
	integer statId;
	boolean result;

	boolean BoardPrepareTileForPlacement_result;
	statId = GetStatIdAt(x, y);
	if (statId > 0)  {
		RemoveStat(statId);
		result = true;
	} else if (statId < 0)  {
		if (! ElementDefs[Board.Tiles[x][y].Element].PlaceableOnTop)
			Board.Tiles[x][y].Element = E_EMPTY;
		result = true;
	} else {       /* statId = 0 (player) cannot be modified */
		result = false;
	}
	BoardDrawTile(x, y);
	BoardPrepareTileForPlacement_result = result;
	return BoardPrepareTileForPlacement_result;
}

void MoveStat(integer statId, integer newX, integer newY) {
	TTile iUnder;
	integer ix, iy;
	integer oldX, oldY;
	integer oldBgColor;

	{
		TStat& with = Board.Stats[statId];
		oldBgColor = Board.Tiles[newX][newY].Color & 0xf0;

		iUnder = Board.Stats[statId].Under;
		Board.Stats[statId].Under = Board.Tiles[newX][newY];

		if (Board.Tiles[with.X][with.Y].Element == E_PLAYER)
			Board.Tiles[newX][newY].Color = Board.Tiles[with.X][with.Y].Color;
		else if (Board.Tiles[newX][newY].Element == E_EMPTY)
			Board.Tiles[newX][newY].Color = Board.Tiles[with.X][with.Y].Color & 0xf;
		else
			Board.Tiles[newX][newY].Color = (Board.Tiles[with.X][with.Y].Color & 0xf)
			                                + (Board.Tiles[newX][newY].Color & 0x70);

		Board.Tiles[newX][newY].Element = Board.Tiles[with.X][with.Y].Element;
		Board.Tiles[with.X][with.Y] = iUnder;

		oldX = with.X;
		oldY = with.Y;
		with.X = newX;
		with.Y = newY;

		BoardDrawTile(with.X, with.Y);
		BoardDrawTile(oldX, oldY);

		if ((statId == 0) && Board.Info.IsDark && (World.Info.TorchTicks > 0))  {
			if ((sqr(oldX-with.X) + sqr(oldY-with.Y)) == 1)  {
				for( ix = (with.X - TORCH_DX - 3); ix <= (with.X + TORCH_DX + 3); ix ++)
					if ((ix >= 1) && (ix <= BOARD_WIDTH))
						for( iy = (with.Y - TORCH_DY - 3); iy <= (with.Y + TORCH_DY + 3); iy ++)
							if ((iy >= 1) && (iy <= BOARD_HEIGHT))
								if ((((sqr(ix-oldX))+(sqr(iy-oldY)*2)) < TORCH_DIST_SQR) ^
								        (((sqr(ix-newX))+(sqr(iy-newY)*2)) < TORCH_DIST_SQR))
									BoardDrawTile(ix, iy);
			} else {
				DrawPlayerSurroundings(oldX, oldY, 0);
				DrawPlayerSurroundings(with.X, with.Y, 0);
			}
		}

	}
}

void PopupPromptString(string question, TString50& buffer) {
	integer x, y;

	video.VideoWriteText(3, 18, 0x4f, TextWindowStrTop);
	video.VideoWriteText(3, 19, 0x4f, TextWindowStrText);
	video.VideoWriteText(3, 20, 0x4f, TextWindowStrSep);
	video.VideoWriteText(3, 21, 0x4f, TextWindowStrText);
	video.VideoWriteText(3, 22, 0x4f, TextWindowStrText);
	video.VideoWriteText(3, 23, 0x4f, TextWindowStrBottom);
	video.VideoWriteText(4 + (TextWindowWidth - length(question)) / 2, 19,
	                     0x4f,
	                     question);
	buffer = "";
	PromptString(10, 22, 0x4f, 0x4e, TextWindowWidth - 16, PROMPT_ANY, buffer);
	for( y = 18; y <= 23; y ++)
		for( x = 3; x <= (TextWindowWidth + 3); x ++)
			BoardDrawTile(x + 1, y + 1);
}

integer Signum(integer val) {
	integer Signum_result;
	if (val > 0)
		Signum_result = 1;
	else if (val < 0)
		Signum_result = -1;
	else
		Signum_result = 0;
	return Signum_result;
}

integer Difference(integer a, integer b) {
	integer Difference_result;
	if ((a - b) >= 0)
		Difference_result = a - b;
	else
		Difference_result = b - a;
	return Difference_result;
}

void GameUpdateSidebar() {
	varying_string<8> numStr;
	integer i;

	if (GameStateElement == E_PLAYER)  {
		if (Board.Info.TimeLimitSec > 0)  {
			video.VideoWriteText(64, 6, 0x1e, "   Time:");
			str(Board.Info.TimeLimitSec - World.Info.BoardTimeSec, numStr);
			video.VideoWriteText(72, 6, 0x1e, numStr + ' ');
		} else {
			SidebarClearLine(6);
		}

		if (World.Info.Health < 0)
			World.Info.Health = 0;

		str(World.Info.Health, numStr);
		video.VideoWriteText(72, 7, 0x1e, numStr + ' ');
		str(World.Info.Ammo, numStr);
		video.VideoWriteText(72, 8, 0x1e, numStr + "  ");
		str(World.Info.Torches, numStr);
		video.VideoWriteText(72, 9, 0x1e, numStr + ' ');
		str(World.Info.Gems, numStr);
		video.VideoWriteText(72, 10, 0x1e, numStr + ' ');
		str(World.Info.Score, numStr);
		video.VideoWriteText(72, 11, 0x1e, numStr + ' ');

		if (World.Info.TorchTicks == 0)
			video.VideoWriteText(75, 9, 0x16, "    ");
		else {
			for( i = 2; i <= 5; i ++) {
				if (i <= ((World.Info.TorchTicks * 5) / TORCH_DURATION))
					video.VideoWriteText(73 + i, 9, 0x16, "\261");
				else
					video.VideoWriteText(73 + i, 9, 0x16, "\260");
			}
		}

		for( i = 1; i <= 7; i ++) {
			if (World.Info.HasKey(i))
				video.VideoWriteText(71 + i, 12, 0x18 + i, ElementDefs[E_KEY].Character);
			else
				video.VideoWriteText(71 + i, 12, 0x1f, " ");
		}

		if (SoundEnabled)
			video.VideoWriteText(65, 15, 0x1f, " Be quiet");
		else
			video.VideoWriteText(65, 15, 0x1f, " Be noisy");

		if (DebugEnabled)  {
			/* TODO: Replace with some interesting stat
			on Linux.*/
			numStr = "lots";
			video.VideoWriteText(69, 4, 0x1e, string('m') + numStr + ' ');
		}
	}
}

void DisplayMessage(integer time, string message) {
	if (GetStatIdAt(0, 0) != -1)  {
		RemoveStat(GetStatIdAt(0, 0));
		BoardDrawBorder();
	}

	if (length(message) != 0)  {
		AddStat(0, 0, E_MESSAGE_TIMER, 0, 1, StatTemplateDefault);
		/*IMP: P2 is a byte, so it can hold a max value of 255.*/
		Board.Stats[Board.StatCount].P2 = Min(255, time / (TickTimeDuration + 1));
		Board.Info.Message = message;
	}
}

void DamageStat(integer attackerStatId) {
	integer oldX, oldY;

	{
		TStat& with = Board.Stats[attackerStatId];
		if (attackerStatId == 0)  {
			if (World.Info.Health > 0)  {
				World.Info.Health = World.Info.Health - 10;

				GameUpdateSidebar();
				DisplayMessage(100, "Ouch!");

				Board.Tiles[with.X][with.Y].Color = 0x70 + (ElementDefs[4].Color % 0x10);

				if (World.Info.Health > 0)  {
					World.Info.BoardTimeSec = 0;
					if (Board.Info.ReenterWhenZapped)  {
						SoundQueue(4, "\40\1\43\1\47\1\60\1\20\1");

						/* Move player to start */
						Board.Tiles[with.X][with.Y].Element = E_EMPTY;
						BoardDrawTile(with.X, with.Y);
						oldX = with.X;
						oldY = with.Y;
						with.X = Board.Info.StartPlayerX;
						with.Y = Board.Info.StartPlayerY;
						DrawPlayerSurroundings(oldX, oldY, 0);
						DrawPlayerSurroundings(with.X, with.Y, 0);

						GamePaused = true;
					}
					SoundQueue(4, "\20\1\40\1\23\1\43\1");
				} else {
					SoundQueue(5,
					           "\40\3\43\3\47\3\60\3\47\3\52\3\62\3\67\3\65\3\70\3\100\3\105\3\20\n");
				}
			}
		} else {
			switch (Board.Tiles[with.X][with.Y].Element) {
			case E_BULLET: SoundQueue(3, "\40\1"); break;
			case E_OBJECT: {; } break;
			default:
				SoundQueue(3, "\100\1\20\1\120\1\60\1");
			}
			RemoveStat(attackerStatId);
		}
	}
}

void BoardDamageTile(integer x, integer y) {
	integer statId;

	statId = GetStatIdAt(x, y);
	if (statId != -1)  {
		DamageStat(statId);
	} else {
		Board.Tiles[x][y].Element = E_EMPTY;
		BoardDrawTile(x, y);
	}
}

void BoardAttack(integer attackerStatId, integer x, integer y) {
	if ((attackerStatId == 0) && (World.Info.EnergizerTicks > 0))  {
		World.Info.Score = ElementDefs[Board.Tiles[x][y].Element].ScoreValue +
		                   World.Info.Score;
		GameUpdateSidebar();
	} else {
		DamageStat(attackerStatId);
	}

	if ((attackerStatId > 0) && (attackerStatId <= CurrentStatTicked))
		CurrentStatTicked = CurrentStatTicked - 1;

	if ((Board.Tiles[x][y].Element == E_PLAYER)
	        && (World.Info.EnergizerTicks > 0))  {
		World.Info.Score =
		    ElementDefs[Board.Tiles[Board.Stats[attackerStatId].X][Board.Stats[attackerStatId].Y].Element]
		    .ScoreValue + World.Info.Score;
		GameUpdateSidebar();
	} else {
		BoardDamageTile(x, y);
		SoundQueue(2, "\20\1");
	}
}

boolean BoardShoot(byte element, integer tx, integer ty, integer deltaX,
                   integer deltaY, integer source) {
	boolean BoardShoot_result;
	if (ElementDefs[Board.Tiles[tx + deltaX][ty + deltaY].Element].Walkable
	        || (Board.Tiles[tx + deltaX][ty + deltaY].Element == E_WATER)) {
		AddStat(tx + deltaX, ty + deltaY, element, ElementDefs[element].Color, 1,
		        StatTemplateDefault);
		{
			TStat& with = Board.Stats[Board.StatCount];
			with.P1 = source;
			with.StepX = deltaX;
			with.StepY = deltaY;
			with.P2 = 100;
		}
		BoardShoot_result = true;
	} else if ((Board.Tiles[tx + deltaX][ty + deltaY].Element == E_BREAKABLE)
	           || (
	               ElementDefs[Board.Tiles[tx + deltaX][ty + deltaY].Element].Destructible
	               && ((Board.Tiles[tx + deltaX][ty + deltaY].Element == E_PLAYER) ==
	                   (boolean)(source))
	               && (World.Info.EnergizerTicks <= 0)
	           )) {
		BoardDamageTile(tx + deltaX, ty + deltaY);
		SoundQueue(2, "\20\1");
		BoardShoot_result = true;
	} else {
		BoardShoot_result = false;
	}
	return BoardShoot_result;
}

void CalcDirectionRnd(integer& deltaX, integer& deltaY) {
	deltaX = Random(3) - 1;

	if (deltaX == 0)
		deltaY = Random(2) * 2 - 1;
	else
		deltaY = 0;
}

void CalcDirectionSeek(integer x, integer y, integer& deltaX,
                       integer& deltaY) {
	deltaX = 0;
	deltaY = 0;

	if ((Random(2) < 1) || (Board.Stats[0].Y == y))
		deltaX = Signum(Board.Stats[0].X - x);

	if (deltaX == 0)
		deltaY = Signum(Board.Stats[0].Y - y);

	if (World.Info.EnergizerTicks > 0)  {
		deltaX = -deltaX;
		deltaY = -deltaY;
	}
}

void TransitionDrawBoardChange() {
	TransitionDrawToFill('\333', 0x5);
	TransitionDrawToBoard();
}

void BoardEnter() {
	Board.Info.StartPlayerX = Board.Stats[0].X;
	Board.Info.StartPlayerY = Board.Stats[0].Y;

	if (Board.Info.IsDark && MessageHintTorchNotShown)  {
		DisplayMessage(200, "Room is dark - you need to light a torch!");
		MessageHintTorchNotShown = false;
	}

	World.Info.BoardTimeSec = 0;
	GameUpdateSidebar();
}

void BoardPassageTeleport(integer x, integer y) {
	integer oldBoard;
	byte col;
	integer ix, iy;
	integer newX, newY;

	col = Board.Tiles[x][y].Color;

	oldBoard = World.Info.CurrentBoard;
	BoardChange(Board.Stats[GetStatIdAt(x, y)].P3);

	newX = 0;
	for( ix = 1; ix <= BOARD_WIDTH; ix ++)
		for( iy = 1; iy <= BOARD_HEIGHT; iy ++)
			if ((Board.Tiles[ix][iy].Element == E_PASSAGE)
			        && (Board.Tiles[ix][iy].Color == col))  {
				newX = ix;
				newY = iy;
			}

	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element = E_EMPTY;
	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Color = 0;
	if (newX != 0)  {
		Board.Stats[0].X = newX;
		Board.Stats[0].Y = newY;
	}

	GamePaused = true;
	SoundQueue(4,
	           "\60\1\64\1\67\1\61\1\65\1\70\1\62\1\66\1\71\1\63\1\67\1\72\1\64\1\70\1\100\1");
	TransitionDrawBoardChange();
	BoardEnter();
}

void GameDebugPrompt() {
	TString50 input;
	integer i;
	boolean toggle;

	input = "";
	SidebarClearLine(4);
	SidebarClearLine(5);

	PromptString(63, 5, 0x1e, 0xf, 11, PROMPT_ANY, input);
	for( i = 1; i <= length(input); i ++)
		input[i] = upcase(input[i]);

	toggle = true;
	if ((input[1] == '+') || (input[1] == '-'))  {
		if (input[1] == '-')
			toggle = false;
		input = copy(input, 2, length(input) - 1);

		if (toggle == true)
			WorldSetFlag(input);
		else
			WorldClearFlag(input);
	}

	DebugEnabled = WorldGetFlagPosition("DEBUG") >= 0;

	if (input == "HEALTH")
		World.Info.Health = World.Info.Health + 50;
	else if (input == "AMMO")
		World.Info.Ammo = World.Info.Ammo + 5;
	else if (input == "KEYS")
		for( i = 1; i <= 7; i ++) World.Info.GiveKey(i);
	else if (input == "TORCHES")
		World.Info.Torches = World.Info.Torches + 3;
	else if (input == "TIME")
		World.Info.BoardTimeSec = World.Info.BoardTimeSec - 30;
	else if (input == "GEMS")
		World.Info.Gems = World.Info.Gems + 5;
	else if (input == "DARK")  {
		Board.Info.IsDark = toggle;
		TransitionDrawToBoard();
	} else if (input == "ZAP")  {
		for( i = 0; i <= 3; i ++) {
			BoardDamageTile(Board.Stats[0].X + NeighborDeltaX[i],
			                Board.Stats[0].Y + NeighborDeltaY[i]);
			Board.Tiles[Board.Stats[0].X + NeighborDeltaX[i]][Board.Stats[0].Y +
			        NeighborDeltaY[i]].Element = E_EMPTY;
			BoardDrawTile(Board.Stats[0].X + NeighborDeltaX[i],
			              Board.Stats[0].Y + NeighborDeltaY[i]);
		}
	}

	SoundQueue(10, "\47\4");
	SidebarClearLine(4);
	SidebarClearLine(5);
	GameUpdateSidebar();
}

void GameAboutScreen() {
	TextWindowDisplayFile("ABOUT.HLP", "About ZZT...");
}

void GamePlayLoop(boolean boardChanged);

static void GameDrawSidebar() {
	SidebarClear();
	SidebarClearLine(0);
	SidebarClearLine(1);
	SidebarClearLine(2);
	video.VideoWriteText(61, 0, 0x1f, "    - - - - -      ");
	video.VideoWriteText(62, 1, 0x70, "      ZZT      ");
	video.VideoWriteText(61, 2, 0x1f, "    - - - - -      ");
	if (GameStateElement == E_PLAYER)  {
		video.VideoWriteText(64, 7, 0x1e, " Health:");
		video.VideoWriteText(64, 8, 0x1e, "   Ammo:");
		video.VideoWriteText(64, 9, 0x1e, "Torches:");
		video.VideoWriteText(64, 10, 0x1e, "   Gems:");
		video.VideoWriteText(64, 11, 0x1e, "  Score:");
		video.VideoWriteText(64, 12, 0x1e, "   Keys:");
		video.VideoWriteText(62, 7, 0x1f, ElementDefs[E_PLAYER].Character);
		video.VideoWriteText(62, 8, 0x1b, ElementDefs[E_AMMO].Character);
		video.VideoWriteText(62, 9, 0x16, ElementDefs[E_TORCH].Character);
		video.VideoWriteText(62, 10, 0x1b, ElementDefs[E_GEM].Character);
		video.VideoWriteText(62, 12, 0x1f, ElementDefs[E_KEY].Character);
		video.VideoWriteText(62, 14, 0x70, " T ");
		video.VideoWriteText(65, 14, 0x1f, " Torch");
		video.VideoWriteText(62, 15, 0x30, " B ");
		video.VideoWriteText(62, 16, 0x70, " H ");
		video.VideoWriteText(65, 16, 0x1f, " Help");
		video.VideoWriteText(67, 18, 0x30, " \30\31\32\33 ");
		video.VideoWriteText(72, 18, 0x1f, " Move");
		video.VideoWriteText(61, 19, 0x70, " Shift \30\31\32\33 ");
		video.VideoWriteText(72, 19, 0x1f, " Shoot");
		video.VideoWriteText(62, 21, 0x70, " S ");
		video.VideoWriteText(65, 21, 0x1f, " Save game");
		video.VideoWriteText(62, 22, 0x30, " P ");
		video.VideoWriteText(65, 22, 0x1f, " Pause");
		video.VideoWriteText(62, 23, 0x70, " Q ");
		video.VideoWriteText(65, 23, 0x1f, " Quit");
	} else if (GameStateElement == E_MONITOR)  {
		SidebarPromptSlider(false, 66, 21, "Game speed:;FS", TickSpeed);
		video.VideoWriteText(62, 21, 0x70, " S ");
		video.VideoWriteText(62, 7, 0x30, " W ");
		video.VideoWriteText(65, 7, 0x1e, " World:");

		if (World.Info.Name.size() != 0)
			video.VideoWriteText(69, 8, 0x1f, World.Info.Name);
		else
			video.VideoWriteText(69, 8, 0x1f, "Untitled");

		video.VideoWriteText(62, 11, 0x70, " P ");
		video.VideoWriteText(65, 11, 0x1f, " Play");
		video.VideoWriteText(62, 12, 0x30, " R ");
		video.VideoWriteText(65, 12, 0x1e, " Restore game");
		video.VideoWriteText(62, 13, 0x70, " Q ");
		video.VideoWriteText(65, 13, 0x1e, " Quit");
		video.VideoWriteText(62, 16, 0x30, " A ");
		video.VideoWriteText(65, 16, 0x1f, " About ZZT!");
		video.VideoWriteText(62, 17, 0x70, " H ");
		video.VideoWriteText(65, 17, 0x1e, " High Scores");

		if (EditorEnabled)  {
			video.VideoWriteText(62, 18, 0x30, " E ");
			video.VideoWriteText(65, 18, 0x1e, " Board Editor");
		}
	}
}

void GamePlayLoop(boolean boardChanged) {
	boolean exitLoop;
	boolean pauseBlink;

	GameDrawSidebar();
	GameUpdateSidebar();

	if (JustStarted)  {
		GameAboutScreen();
		if (length(StartupWorldFileName) != 0)  {
			SidebarClearLine(8);
			video.VideoWriteText(69, 8, 0x1f, StartupWorldFileName);
			if (! WorldLoad(std::string(StartupWorldFileName), ".ZZT"))  WorldCreate();
		}
		ReturnBoardId = World.Info.CurrentBoard;
		BoardChange(0);
		JustStarted = false;
	}

	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element = GameStateElement;
	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Color =
	    ElementDefs[GameStateElement].Color;

	if (GameStateElement == E_MONITOR)  {
		DisplayMessage(0, "");
		video.VideoWriteText(62, 5, 0x1b, "Pick a command:");
	}

	if (boardChanged)
		TransitionDrawBoardChange();

	TickTimeDuration = TickSpeed * 2;
	GamePlayExitRequested = false;
	exitLoop = false;

	CurrentTick = Random(100);
	CurrentStatTicked = Board.StatCount + 1;

	pauseBlink = true;

	do {
		if (GamePaused)  {
			if (SoundHasTimeElapsed(TickTimeCounter, 25))
				pauseBlink = ! pauseBlink;

			if (pauseBlink)  {
				video.VideoWriteText(Board.Stats[0].X - 1, Board.Stats[0].Y - 1,
				                     ElementDefs[E_PLAYER].Color, ElementDefs[E_PLAYER].Character);
			} else {
				if (Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element == E_PLAYER)
					video.VideoWriteText(Board.Stats[0].X - 1, Board.Stats[0].Y - 1, 0xf, " ");
				else
					BoardDrawTile(Board.Stats[0].X, Board.Stats[0].Y);
			}

			video.VideoWriteText(64, 5, 0x1f, "Pausing...");
			InputUpdate();

			if (InputKeyPressed == E_KEY_ESCAPE) {
				GamePromptEndPlay();
            }

			if ((InputDeltaX != 0) || (InputDeltaY != 0))  {
				ElementDefs[Board.Tiles[Board.Stats[0].X + InputDeltaX][Board.Stats[0].Y +
				                                         InputDeltaY].Element].TouchProc(
				                Board.Stats[0].X + InputDeltaX, Board.Stats[0].Y + InputDeltaY, 0,
				                InputDeltaX, InputDeltaY);
			}

			if (((InputDeltaX != 0) || (InputDeltaY != 0))
			        && ElementDefs[Board.Tiles[Board.Stats[0].X + InputDeltaX][Board.Stats[0].Y
			                                                    + InputDeltaY].Element].Walkable) {
				/* Move player */
				if (Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element == E_PLAYER)
					MoveStat(0, Board.Stats[0].X + InputDeltaX,
					         Board.Stats[0].Y + InputDeltaY);
				else {
					BoardDrawTile(Board.Stats[0].X, Board.Stats[0].Y);
					Board.Stats[0].X = Board.Stats[0].X + InputDeltaX;
					Board.Stats[0].Y = Board.Stats[0].Y + InputDeltaY;
					Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element = E_PLAYER;
					Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Color =
					    ElementDefs[E_PLAYER].Color;
					BoardDrawTile(Board.Stats[0].X, Board.Stats[0].Y);
					DrawPlayerSurroundings(Board.Stats[0].X, Board.Stats[0].Y, 0);
					DrawPlayerSurroundings(Board.Stats[0].X - InputDeltaX,
					                       Board.Stats[0].Y - InputDeltaY, 0);
				}

				/* Unpause */
				GamePaused = false;
				SidebarClearLine(5);
				CurrentTick = Random(100);
				CurrentStatTicked = Board.StatCount + 1;
				World.Info.IsSave = true;
			}

		} else {       /* not GamePaused */
			if (CurrentStatTicked <= Board.StatCount)  {
				{
					TStat& with = Board.Stats[CurrentStatTicked];
					if ((with.Cycle != 0)
					        && ((CurrentTick % with.Cycle) == (CurrentStatTicked % with.Cycle)))
						ElementDefs[Board.Tiles[with.X][with.Y].Element].TickProc(
						    CurrentStatTicked);

					CurrentStatTicked = CurrentStatTicked + 1;
				}
			}
		}

		if ((CurrentStatTicked > Board.StatCount) && ! GamePlayExitRequested)  {
			/* all stats ticked */
			if (SoundHasTimeElapsed(TickTimeCounter, TickTimeDuration))  {
				/* next cycle */
				CurrentTick = CurrentTick + 1;
				if (CurrentTick > 420)
					CurrentTick = 1;
				CurrentStatTicked = 0;

				InputUpdate();
			}
		}

        // Imported from fuzz branch for easier debugging:

        /* Crash if the invariant that the player (or monitor) must exist
            and be at the X,Y given by stat 0 is violated. We have to check
            for both player and monitor no matter what the GameStateElement
            is in order to support Chronos' Forced Play hack. */
        if (!ValidCoord(Board.Stats[0].X, Board.Stats[0].Y)) {
            throw std::logic_error("game.cxx: Player or Monitor is off-board."
                " This should never happen.");
        }

        byte playerTileElem = Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element;
        if (playerTileElem != E_PLAYER && playerTileElem != E_MONITOR) {
            throw std::logic_error("game.cxx: Board has no Player or Monitor."
                " This should never happen.");
        }

	} while (!((exitLoop || GamePlayExitRequested) && GamePlayExitRequested));

	SoundClearQueue();

	if (GameStateElement == E_PLAYER)  {
        // XXX: Once we add in editor.cxx
        // Should high score routines be in editor anyway?
		/*if (World.Info.Health <= 0)  {
			HighScoresAdd(World.Info.Score);
		}*/
	} else if (GameStateElement == E_MONITOR)  {
		SidebarClearLine(5);
	}

	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element = E_PLAYER;
	Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Color =
	    ElementDefs[E_PLAYER].Color;

	SoundBlockQueueing = false;
}

void GameTitleLoop() {
	boolean boardChanged;
	boolean startPlay;

	GameTitleExitRequested = false;
	JustStarted = true;
	ReturnBoardId = 0;
	boardChanged = true;
	do {
		BoardChange(0);
		do {
			GameStateElement = E_MONITOR;
			startPlay = false;
			GamePaused = false;
			GamePlayLoop(boardChanged);
			boardChanged = false;

			switch (keyUpCase(InputKeyPressed)) {
			case 'W': {
				if (GameWorldLoad(".ZZT"))  {
					ReturnBoardId = World.Info.CurrentBoard;
					boardChanged = true;
				}
			}
			break;
			case 'P': {
				if (World.Info.IsSave && ! DebugEnabled)  {
					startPlay = WorldLoad(World.Info.Name, ".ZZT");
					ReturnBoardId = World.Info.CurrentBoard;
				} else {
					startPlay = true;
				}
				if (startPlay)  {
					BoardChange(ReturnBoardId);
					BoardEnter();
				}
			}
			break;
			case 'A': {
				GameAboutScreen();
			}
			break;
            // XXX: Once we add in editor.cxx
			/*case 'E': if (EditorEnabled)  {
					EditorLoop();
					ReturnBoardId = World.Info.CurrentBoard;
					boardChanged = true;
				}
				break;*/
			case 'S': {
				SidebarPromptSlider(true, 66, 21, "Game speed:;FS", TickSpeed);
				InputKeyPressed = '\0';
			}
			break;
			case 'R': {
				if (GameWorldLoad(".SAV"))  {
					ReturnBoardId = World.Info.CurrentBoard;
					BoardChange(ReturnBoardId);
					startPlay = true;
				}
			}
			break;
            // XXX: Once we add in editor.cxx
			/*case 'H': {
				HighScoresLoad();
				HighScoresDisplay(1);
			}*/
			break;
			case '|': {
				GameDebugPrompt();
			}
			break;
			case E_KEY_ESCAPE: case 'Q': {
				GameTitleExitRequested = SidebarPromptYesNo("Quit ZZT? ", true);
			}
			break;
			}

			if (startPlay)  {
				GameStateElement = E_PLAYER;
				GamePaused = true;
				GamePlayLoop(true);
				boardChanged = true;
			}
		} while (!(boardChanged || GameTitleExitRequested));
	} while (!GameTitleExitRequested);
}

void GamePrintRegisterMessage() {
	std::string s;
	std::ifstream f;
	integer i;
	integer ix, iy;
	integer color;
	boolean isReading;
	string * strPtr;

//	SetCBreak(false);
	s = "END" + itos(49 + Random(4)) + ".MSG";
	iy = 0;
	color = 0xf;
	word actuallyRead;

	// TODO: Fix resource file reading.

	for( i = 1; i <= ResourceDataHeader.EntryCount; i ++) {
		if (s == ResourceDataHeader.Name[i])  {
			f = OpenForRead(ResourceDataFileName);
			f.seekg(ResourceDataHeader.FileOffset[i]); // * record length?

			isReading = true;
			while (errno == 0 && isReading)  {
				char scratch[256];
				f.read(scratch, 1); // is this also in record terms?
				s = scratch;
				if (s.size() == 0)  {
					color -= 1;
				} else {
					f.read(scratch+1, s.size());
					s = scratch;
					if (s[0] != '@')
						video.VideoWriteText(0, iy, color, s);
					else
						isReading = false;
				}
				iy += 1;
			}

			f.close();
			video.VideoWriteText(28, 24, 0x1f, "Press any key to exit...");
			TextColor(LightGray);

			ReadKeyBlocking();

			video.VideoWriteText(28, 24, 0, "                        ");
			GotoXY(1, 23);
		}
	}
}

class unit_Game_initialize {
public: unit_Game_initialize();
};
static unit_Game_initialize Game_constructor;

unit_Game_initialize::unit_Game_initialize() {
	;
}
