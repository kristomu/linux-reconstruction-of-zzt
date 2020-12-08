#include "ptoc.h"

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
#define __TxtWind_implementation__


#include "txtwind.h"

/*#include "Crt.h"*/
/*#include "Printer.h"*/
#include "gamevars.h"
#include "fileops.h"

#include "video.h"
#include "hardware.h"

string UpCaseString(string input) {
	integer i;

	string UpCaseString_result;
	for( i = 1; i <= length(input); i ++)
		input[i] = upcase(input[i]);
	UpCaseString_result = input;
	return UpCaseString_result;
}

void TextWindowInitState(TTextWindowState& state) {
	{
		state.LineCount = 0;
		state.LinePos = 1;
		state.LoadedFilename = "";
	}
}

void TextWindowDrawTitle(integer color, TTextWindowLine title) {
	video.VideoWriteText(TextWindowX + 2, TextWindowY + 1, color,
	                     TextWindowStrInnerEmpty);
	video.VideoWriteText(TextWindowX + ((TextWindowWidth - length(title)) / 2),
	                     TextWindowY + 1, color, title);
}

void TextWindowDrawOpen(TTextWindowState& state) {
	int ix, iy;

	{
		video.VideoCopy(TextWindowX, TextWindowY, TextWindowWidth,
		                TextWindowHeight+1, false);

		for( iy = (TextWindowHeight / 2); iy >= 0; iy --) {
			video.VideoWriteText(TextWindowX, TextWindowY + iy + 1, 0xf,
			                     TextWindowStrText);
			video.VideoWriteText(TextWindowX, TextWindowY + TextWindowHeight - iy - 1,
			                     0xf, TextWindowStrText);
			video.VideoWriteText(TextWindowX, TextWindowY + iy, 0xf, TextWindowStrTop);
			video.VideoWriteText(TextWindowX, TextWindowY + TextWindowHeight - iy, 0xf,
			                     TextWindowStrBottom);
			Delay(25);
            video.VideoRefresh();
		}

		video.VideoWriteText(TextWindowX, TextWindowY + 2, 0xf, TextWindowStrSep);
		TextWindowDrawTitle(0x1e, state.Title);
	}
}

void TextWindowDrawClose(TTextWindowState& state) {
	integer ix, iy;
	integer unk1, unk2;

	{
		for( iy = 0; iy <= (TextWindowHeight / 2); iy ++) {
			video.VideoWriteText(TextWindowX, TextWindowY + iy, 0xf, TextWindowStrTop);
			video.VideoWriteText(TextWindowX, TextWindowY + TextWindowHeight - iy, 0xf,
			                     TextWindowStrBottom);
			Delay(18);
			/* Replace upper line with background. */
			video.VideoCopy(TextWindowX, TextWindowY + iy, TextWindowWidth, 1,
			                true);
			/* Replace lower line with background. */
			video.VideoCopy(TextWindowX, TextWindowY + TextWindowHeight - iy,
			                TextWindowWidth, 1, true);
            video.VideoRefresh();
		}
	}
}

void TextWindowDrawLine(TTextWindowState& state, integer lpos,
                        boolean withoutFormatting, boolean viewingFile) {
	integer lineY;
	integer textOffset, textColor, textX;

	{
		lineY = ((TextWindowY + lpos) - state.LinePos) + (TextWindowHeight / 2) +
		        1;
		if (lpos == state.LinePos)
			video.VideoWriteText(TextWindowX + 2, lineY, 0x1c,
			                     TextWindowStrInnerArrows);
		else
			video.VideoWriteText(TextWindowX + 2, lineY, 0x1e,
			                     TextWindowStrInnerEmpty);
		if ((lpos > 0) && (lpos <= state.LineCount))  {
			if (withoutFormatting)  {
				video.VideoWriteText(TextWindowX + 4, lineY, 0x1e, *state.Lines[lpos]);
			} else {
				textOffset = 1;
				textColor = 0x1e;
				textX = TextWindowX + 4;
				if (length(*state.Lines[lpos]) > 0)  {
					switch ((*state.Lines[lpos])[1]) {
					case '!': {
						textOffset = pos(";", *state.Lines[lpos]) + 1;
						video.VideoWriteText(textX + 2, lineY, 0x1d, "\20");
						textX = textX + 5;
						textColor = 0x1f;
					}
					break;
					case ':': {
						textOffset = pos(";", *state.Lines[lpos]) + 1;
						textColor = 0x1f;
					}
					break;
					case '$': {
						textOffset = 2;
						textColor = 0x1f;
						textX = (textX - 4) + ((TextWindowWidth - length(*state.Lines[lpos])) / 2);
					}
					break;
					}
				}
				if (textOffset > 0)  {
					video.VideoWriteText(textX, lineY, textColor,
					                     copy(*state.Lines[lpos], textOffset,
					                          length(*state.Lines[lpos]) - textOffset + 1));
				}
			}
		} else if ((lpos == 0) || (lpos == (state.LineCount + 1)))  {
			video.VideoWriteText(TextWindowX + 2, lineY, 0x1e, TextWindowStrInnerSep);
		} else if ((lpos == -4) && viewingFile)  {
			video.VideoWriteText(TextWindowX + 2, lineY, 0x1a,
			                     "   Use            to view text,");
			video.VideoWriteText(TextWindowX + 2 + 7, lineY, 0x1f, "\30 \31, Enter");
		} else if ((lpos == -3) && viewingFile)  {
			video.VideoWriteText(TextWindowX + 2 + 1, lineY, 0x1a,
			                     "                 to print.");
			video.VideoWriteText(TextWindowX + 2 + 12, lineY, 0x1f, "Alt-P");
		}
	}
}

void TextWindowDraw(TTextWindowState& state, boolean withoutFormatting,
                    boolean viewingFile) {
	integer i;
	integer unk1;

	for( i = 0; i <= (TextWindowHeight - 4); i ++)
		TextWindowDrawLine(state, state.LinePos - (TextWindowHeight / 2) + i + 2,
		                   withoutFormatting, viewingFile);
	TextWindowDrawTitle(0x1e, state.Title);
}

void TextWindowAppend(TTextWindowState& state, TTextWindowLine line) {
	{
		state.LineCount = state.LineCount + 1;
		state.Lines[state.LineCount] = new TTextWindowLine;
		*state.Lines[state.LineCount] = line;
	}
}

void TextWindowFree(TTextWindowState& state) {
	{
		while (state.LineCount > 0)  {
			delete state.Lines[state.LineCount];
			state.LineCount = state.LineCount - 1;
		}
		state.LoadedFilename = "";
	}
}

void TextWindowPrint(TTextWindowState& state) {
	integer iLine, iChar;
	string line;
    text Lst;

	{
        assign(Lst, "PRINTOUT.DAT");
		OpenForWrite(Lst); /*??? What is Lst? Lst is the printer out*/
		for( iLine = 1; iLine <= state.LineCount; iLine ++) {
			line = *state.Lines[iLine];
			if (length(line) > 0)  {
				switch (line[1]) {
				case '$': {
					Delete(line, 1, 1);
					for( iChar = ((80 - length(line)) / 2); iChar >= 1; iChar --)
						line = string(' ') + line;
				}
				break;
				case '!': case ':': {
					iChar = pos(";", line);
					if (iChar > 0)
						Delete(line, 1, iChar);
					else
						line = "";
				}
				break;
				default:
					line = string("          ") + line;
				}
			}
			Lst << line << NL;
			if (ioResult != 0)  {
				close(Lst);
				return;
			}
		}
		if (state.LoadedFilename == "ORDER.HLP")  {
			Lst << *OrderPrintId << NL;
		}
		Lst << chr(12); /* form feed */
		close(Lst);
	}
}

void TextWindowSelect(TTextWindowState& state, boolean hyperlinkAsSelect,
                      boolean viewingFile) {
	integer newLinePos;
	integer unk1;
	integer iLine, iChar;
	varying_string<20> pointerStr;



	{
		TextWindowRejected = false;
		state.Hyperlink = "";
		TextWindowDraw(state, false, viewingFile);
		do {
            InputReadWaitKey();
			newLinePos = state.LinePos;
			if (InputDeltaY != 0)  {
				newLinePos = newLinePos + InputDeltaY;
			} else if (InputShiftPressed || (InputKeyPressed == E_KEY_ENTER))  {
				InputShiftAccepted = true;
				/* IMP: Fix potential out-of-bounds access. */
				if ((length(*state.Lines[state.LinePos]) > 0)
				        && (((*state.Lines[state.LinePos])[1]) == '!'))  {
					pointerStr = copy(*state.Lines[state.LinePos], 2,
					                  length(*state.Lines[state.LinePos]) - 1);

					if (pos(";", pointerStr) > 0)  {
						pointerStr = copy(pointerStr, 1, pos(";", pointerStr) - 1);
					}

					if (pointerStr[1] == '-')  {
						Delete(pointerStr, 1, 1);
						TextWindowFree(state);
						TextWindowOpenFile(pointerStr, state);
						if (state.LineCount == 0)
							return;
						else {
							viewingFile = true;
							newLinePos = state.LinePos;
							TextWindowDraw(state, false, viewingFile);
							InputKeyPressed = '\0';
							InputShiftPressed = false;
						}
					} else {
						if (hyperlinkAsSelect)  {
							state.Hyperlink = pointerStr;
						} else {
							pointerStr = string(':') + pointerStr;
							for( iLine = 1; iLine <= state.LineCount; iLine ++) {
								if (length(pointerStr) > length(*state.Lines[iLine]))  {
									;
								} else {
									for( iChar = 1; iChar <= length(pointerStr); iChar ++) {
										if (upcase(pointerStr[iChar]) != upcase((*state.Lines[iLine])[iChar]))
											goto LLabelNotMatched;
									}
									newLinePos = iLine;
									InputKeyPressed = '\0';
									InputShiftPressed = false;
									goto LLabelMatched;
LLabelNotMatched:;
								}
							}
						}
					}
				}
			} else {
				if (InputKeyPressed == E_KEY_PAGE_UP)
					newLinePos = state.LinePos - TextWindowHeight + 4;
				else if (InputKeyPressed == E_KEY_PAGE_DOWN)
					newLinePos = state.LinePos + TextWindowHeight - 4;
				else if (InputKeyPressed == E_KEY_ALT_P)
					TextWindowPrint(state);
			}

LLabelMatched:
			if (newLinePos < 1)
				newLinePos = 1;
			else if (newLinePos > state.LineCount)
				newLinePos = state.LineCount;

			if (newLinePos != state.LinePos)  {
				state.LinePos = newLinePos;
				TextWindowDraw(state, false, viewingFile);
				/* IMP: Fix potential out-of-bounds access.*/
				if ((length(*state.Lines[state.LinePos]) > 0)
				        && (((*state.Lines[state.LinePos])[1]) == '!'))
					if (hyperlinkAsSelect)
						TextWindowDrawTitle(0x1e, "\256Press ENTER to select this\257");
					else
						TextWindowDrawTitle(0x1e, "\256Press ENTER for more info\257");
			}
			if (InputJoystickMoved)  {
				Delay(35);
			}
		} while (!((InputKeyPressed == E_KEY_ESCAPE)
		           || (InputKeyPressed == E_KEY_ENTER) || InputShiftPressed));
		if (InputKeyPressed == E_KEY_ESCAPE)  {
			InputKeyPressed = '\0';
			TextWindowRejected = true;
		}
	}
}

void TextWindowSort(TTextWindowState& state);


static void Swap(TTextWindowLine*& a, TTextWindowLine*& b) {
	TTextWindowLine* c;

	c = a;
	a = b;
	b = c;
}



/* Returns the location of the pivot after separating. */
static integer Partition(TTextWindowState& state, integer low,
                         integer high) {
	TTextWindowLine* pivot;
	integer i, j;

	/* Choose a random pivot. A cannae be bothered to do anything
	  faster/more sophisticated. */
	integer Partition_result;
	pivot = state.Lines[Random(high-low) + low];
	i = low;
	j = high;

	do {
		while (*state.Lines[i] < *pivot)  i += 1;
		while (*state.Lines[j] > *pivot)  j -= 1;

		if (i <= j)  {
			Swap(state.Lines[i], state.Lines[j]);

			i += 1;
			j -= 1;
		}
	} while (!(i > j));

	Partition_result = i;
	return Partition_result;
}



static void Quicksort(TTextWindowState& state, integer low,
                      integer high) {
	integer pivotPt;

	if (low < high)  {
		pivotPt = Partition(state, low, high);
		Quicksort(state, low, pivotPt-1);
		Quicksort(state, pivotPt, high);
	}
}

void TextWindowSort(TTextWindowState& state) {
	integer i, j;
	integer smallestIdx;


	Quicksort(state, 1, state.LineCount);
}

void TextWindowEdit(TTextWindowState& state);

static integer newLinePos;

static void DeleteCurrLine(TTextWindowState& state) {
	integer i;

	{
		if (state.LineCount > 1)  {
			delete state.Lines[state.LinePos];
			for( i = (state.LinePos + 1); i <= state.LineCount; i ++)
				state.Lines[i - 1] = state.Lines[i];

			state.LineCount = state.LineCount - 1;
			if (state.LinePos > state.LineCount)
				newLinePos = state.LineCount;
			else
				TextWindowDraw(state, true, false);
		} else {
			*state.Lines[1] = "";
		}
	}
}

void TextWindowEdit(TTextWindowState& state) {
	boolean insertMode;
	integer charPos;
	integer i;

	{
		if (state.LineCount == 0)
			TextWindowAppend(state, "");
		insertMode = true;
		state.LinePos = 1;
		charPos = 1;
		TextWindowDraw(state, true, false);
		do {
			if (insertMode)
				video.VideoWriteText(77, 14, 0x1e, "on ");
			else
				video.VideoWriteText(77, 14, 0x1e, "off");

			if (charPos >= (length(*state.Lines[state.LinePos]) + 1))  {
				charPos = length(*state.Lines[state.LinePos]) + 1;
				video.VideoWriteText(charPos + TextWindowX + 3,
				                     TextWindowY + (TextWindowHeight / 2) + 1,
				                     0x70, " ");
			} else {
				video.VideoWriteText(charPos + TextWindowX + 3,
				                     TextWindowY + (TextWindowHeight / 2) + 1,
				                     0x70, (*state.Lines[state.LinePos])[charPos]);
			}
			InputReadWaitKey();
			newLinePos = state.LinePos;
			switch (InputKeyPressed) {
			case E_KEY_UP: newLinePos = state.LinePos - 1; break;
			case E_KEY_DOWN: newLinePos = state.LinePos + 1; break;
			case E_KEY_PAGE_UP: newLinePos = state.LinePos - TextWindowHeight + 4; break;
			case E_KEY_PAGE_DOWN: newLinePos = state.LinePos + TextWindowHeight - 4;
				break;
			/*IMP: END and HOME*/
			case E_KEY_END: charPos = length(*state.Lines[state.LinePos]) + 1; break;
			case E_KEY_HOME: charPos = 1; break;
			case E_KEY_RIGHT: {
				charPos = charPos + 1;
				if (charPos > (length(*state.Lines[state.LinePos]) + 1))  {
					charPos = 1;
					newLinePos = state.LinePos + 1;
				}
			}
			break;
			case E_KEY_LEFT: {
				charPos = charPos - 1;
				if (charPos < 1)  {
					charPos = TextWindowWidth;
					newLinePos = state.LinePos - 1;
				}
			}
			break;
			case E_KEY_ENTER: {
				if (state.LineCount < MAX_TEXT_WINDOW_LINES)  {
					for( i = state.LineCount; i >= (state.LinePos + 1); i --)
						state.Lines[i + 1] = state.Lines[i];

					state.Lines[state.LinePos + 1] = new TTextWindowLine;
					*state.Lines[state.LinePos + 1]
					    = copy(*state.Lines[state.LinePos], charPos,
					           length(*state.Lines[state.LinePos]) - charPos + 1);
					*state.Lines[state.LinePos]
					    = copy(*state.Lines[state.LinePos], 1, charPos - 1);

					newLinePos = state.LinePos + 1;
					charPos = 1;
					state.LineCount = state.LineCount + 1;
				}
			}
			break;
			case E_KEY_BACKSPACE: {
				if (charPos > 1)  {
					*state.Lines[state.LinePos] =
					    copy(*state.Lines[state.LinePos], 1, charPos - 2)
					    + copy(*state.Lines[state.LinePos], charPos,
					           length(*state.Lines[state.LinePos]) - charPos + 1);
					charPos = charPos - 1;
				} else if (length(*state.Lines[state.LinePos]) == 0)  {
					DeleteCurrLine(state);
					newLinePos = state.LinePos - 1;
					charPos = TextWindowWidth;
				}
			}
			break;
			case E_KEY_INSERT: {
				insertMode = ! insertMode;
			}
			break;
			case E_KEY_DELETE: {
				*state.Lines[state.LinePos] =
				    copy(*state.Lines[state.LinePos], 1, charPos - 1)
				    + copy(*state.Lines[state.LinePos], charPos + 1,
				           length(*state.Lines[state.LinePos]) - charPos);
			}
			break;
			case E_KEY_CTRL_Y: {
				DeleteCurrLine(state);
			}
			break;
			default:
				if ((InputKeyPressed >= '\40') && (charPos < (TextWindowWidth - 7)))  {
					if (! insertMode)  {
						*state.Lines[state.LinePos] = copy(*state.Lines[state.LinePos], 1,
						                                   charPos - 1)
						                              + (char)(InputKeyPressed)
						                              + copy(*state.Lines[state.LinePos], charPos + 1,
						                                     length(*state.Lines[state.LinePos]) - charPos);
						charPos = charPos + 1;
					} else {
						if (length(*state.Lines[state.LinePos]) < (TextWindowWidth - 8))  {
							*state.Lines[state.LinePos] = copy(*state.Lines[state.LinePos], 1,
							                                   charPos - 1)
							                              + (char)InputKeyPressed
							                              + copy(*state.Lines[state.LinePos], charPos,
							                                     length(*state.Lines[state.LinePos]) - charPos + 1);
							charPos = charPos + 1;
						}
					}
				}
			}

			if (newLinePos < 1)
				newLinePos = 1;
			else if (newLinePos > state.LineCount)
				newLinePos = state.LineCount;

			if (newLinePos != state.LinePos)  {
				state.LinePos = newLinePos;
				TextWindowDraw(state, true, false);
			} else {
				TextWindowDrawLine(state, state.LinePos, true, false);
			}
		} while (!(InputKeyPressed == E_KEY_ESCAPE));

		if (length(*state.Lines[state.LineCount]) == 0)  {
			delete state.Lines[state.LineCount];
			state.LineCount = state.LineCount - 1;
		}
	}
}

void TextWindowOpenFile(TTextWindowLine filename,
                        TTextWindowState& state) {
	untyped_file f;
	text tf;
	integer i;
	integer entryPos;
	boolean retVal;
	TTextWindowLine* line;
	byte lineLen;

	{
		retVal = true;
		for( i = 1; i <= length(filename); i ++)
			retVal = retVal && (filename[i] != '.');
		if (retVal)
			filename = filename + ".HLP";

		if (filename[1] == '*')  {
			filename = copy(filename, 2, length(filename) - 1);
			entryPos = -1;
		} else {
			entryPos = 0;
		}

		TextWindowInitState(state);
		state.LoadedFilename = UpCaseString(filename);
        word how_many = 1;
		if (ResourceDataHeader.EntryCount == 0)  {
			assign(f, ResourceDataFileName);
			OpenForRead(f, 1);
			if (ioResult == 0)
				BlockRead(f, &ResourceDataHeader, sizeof(ResourceDataHeader), how_many);
			if (ioResult != 0)
				ResourceDataHeader.EntryCount = -1;
		}

		if (entryPos == 0)  {
			for( i = 1; i <= ResourceDataHeader.EntryCount; i ++) {
				if (UpCaseString(ResourceDataHeader.Name[i]) == UpCaseString(filename))
					entryPos = i;
			}
		}

		if (entryPos <= 0)  {
			assign(tf, filename);
			OpenForRead(tf);
			if (ioResult == 0)  {
				while ((ioResult == 0) && (! eof(tf)))  {
					state.LineCount += 1;
					state.Lines[state.LineCount] = new TTextWindowLine;
					tf >> *state.Lines[state.LineCount] >> NL;
				}
				close(tf);
			}
		} else {
			assign(f, ResourceDataFileName);
			OpenForRead(f, 1);
			seek(f, ResourceDataHeader.FileOffset[entryPos]);
            word actually_read;
			if (ioResult == 0)  {
				retVal = true;
				while ((ioResult == 0) && retVal)  {
					state.LineCount += 1;
					state.Lines[state.LineCount] = new TTextWindowLine;

					BlockRead(f, state.Lines[state.LineCount], 1, actually_read);
					line = state.Lines[state.LineCount] + 1;
					lineLen = ord((*state.Lines[state.LineCount])[0]);
					if (lineLen == 0)  {
						*state.Lines[state.LineCount] = "";
					} else {
						BlockRead(f, line, ord((*state.Lines[state.LineCount])[0]),
                            actually_read);
					}

					if (*state.Lines[state.LineCount] == '@')  {
						retVal = false;
						*state.Lines[state.LineCount] = "";
					}
				}
				close(f);
			}
		}
	}
}

void TextWindowSaveFile(TTextWindowLine filename,
                        TTextWindowState& state) {
	text f;
	integer i;

	{
		assign(f, filename);
		OpenForWrite(f);
		if (ioResult != 0)  return;

		for( i = 1; i <= state.LineCount; i ++) {
			f << *state.Lines[i] << NL;
			if (ioResult != 0)  return;
		}

		close(f);
	}
}

void TextWindowDisplayFile(string filename, string title) {
	TTextWindowState state;

	state.Title = title;
	TextWindowOpenFile(filename, state);
	state.Selectable = false;
	if (state.LineCount > 0)  {
		TextWindowDrawOpen(state);
		TextWindowSelect(state, false, true);
		TextWindowDrawClose(state);
	}
	TextWindowFree(state);
}

void TextWindowInit(integer x, integer y, integer width, integer height) {
	integer i;

	TextWindowX = x;
	TextWindowWidth = width;
	TextWindowY = y;
	TextWindowHeight = height;
	TextWindowStrInnerEmpty = "";
	TextWindowStrInnerLine = "";
	for( i = 1; i <= (TextWindowWidth - 5); i ++) {
		TextWindowStrInnerEmpty = TextWindowStrInnerEmpty + ' ';
		TextWindowStrInnerLine = TextWindowStrInnerLine + '\315';
	}
	TextWindowStrTop    = string("\306\321") + TextWindowStrInnerLine.c_str()  + '\321'
	                      + '\265';
	TextWindowStrBottom = string("\306\317") + TextWindowStrInnerLine.c_str()  + '\317'
	                      + '\265';
	TextWindowStrSep    =  string(" \306") + TextWindowStrInnerLine.c_str()  + '\265' +
	                       ' ';
	TextWindowStrText   =  string(" \263") + TextWindowStrInnerEmpty.c_str() + '\263' +
	                       ' ';
	TextWindowStrInnerArrows = TextWindowStrInnerEmpty;
	TextWindowStrInnerArrows[1] = '\257';
	TextWindowStrInnerArrows[length(TextWindowStrInnerArrows.c_str())] = '\256';
	TextWindowStrInnerSep = TextWindowStrInnerEmpty;
	for( i = 1; i < (TextWindowWidth / 5); i ++)
		TextWindowStrInnerSep[i * 5 + ((TextWindowWidth % 5) / 2)] = '\7';
}

class unit_TxtWind_initialize {
public: unit_TxtWind_initialize();
};
static unit_TxtWind_initialize TxtWind_constructor;

unit_TxtWind_initialize::unit_TxtWind_initialize() {
	ResourceDataFileName = "";
	ResourceDataHeader.EntryCount = 0;
}
