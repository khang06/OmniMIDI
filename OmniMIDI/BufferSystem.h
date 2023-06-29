/*
OmniMIDI buffer system
Some code has been optimized by Sono (SonoSooS), the old one has been commented out
*/
#pragma once
#define SMALLBUFFER 2

int __inline BufferCheck(void) {
	//return (EVBuffer.ReadHead != EVBuffer.WriteHead);
	if (EVBuffer.ReadHead == EVBuffer.WriteHeadCached) {
		EVBuffer.WriteHeadCached = EVBuffer.WriteHead;
		if (EVBuffer.ReadHead == EVBuffer.WriteHeadCached) {
			return false;
		}
	}
	return true;
}

BOOL __inline CheckIfEventIsToIgnore(DWORD dwParam1)
{
	/*
	if (ignorenotes1) {
		if (((LOWORD(dwParam1) & 0xF0) == 128 || (LOWORD(dwParam1) & 0xF0) == 144)
			&& ((HIWORD(dwParam1) & 0xFF) >= lovel && (HIWORD(dwParam1) & 0xFF) <= hivel)) {
			PrintToConsole(FOREGROUND_RED, dwParam1, "Ignored NoteON/NoteOFF MIDI event.");
			return TRUE;
		}
		else {
			if (limit88) {
				if ((Between(status, 0x80, 0x8f) && (status != 0x89)) || (Between(status, 0x90, 0x9f) && (status != 0x99))) {
					if (!(note >= 21 && note <= 108)) {
						PrintToConsole(FOREGROUND_RED, dwParam1, "Ignored NoteON/NoteOFF MIDI event.");
						return TRUE;
					}
				}
			}
		}
	}
	else {
		if (limit88 == 1) {
			if ((Between(status, 0x80, 0x8f) && (status != 0x89)) || (Between(status, 0x90, 0x9f) && (status != 0x99))) {
				if (!(note >= 21 && note <= 108)) {
					PrintToConsole(FOREGROUND_RED, dwParam1, "Ignored NoteON/NoteOFF MIDI event.");
					return TRUE;
				}
			}
		}
	}

	Understandable version of what the following function does
	*/

	if (ManagedSettings.IgnoreNotesBetweenVel && !((dwParam1 - 0x80) & 0xE0)
		&& ((HIWORD(dwParam1) & 0xFF) >= ManagedSettings.MinVelIgnore && (HIWORD(dwParam1) & 0xFF) <= ManagedSettings.MaxVelIgnore))
	{
		PrintMessageToDebugLog("CheckIfEventIsToIgnoreFunc", "Ignored NoteON/NoteOFF MIDI event.");
		return TRUE;
	}

	if (ManagedSettings.LimitTo88Keys && (!((dwParam1 - 0x80) & 0xE0) && dwParam1 != 0x89))
	{
		if (!(((dwParam1 >> 8) & 0xFF) >= 21 && ((dwParam1 >> 8) & 0xFF) <= 108))
		{
			PrintMessageToDebugLog("CheckIfEventIsToIgnoreFunc", "Ignored NoteON/NoteOFF MIDI event.");
			return TRUE;
		}
	}

	return FALSE;
}

DWORD __inline ReturnEditedEvent(DWORD dwParam1) {
	// SETSTATUS(dwParam1, status);

	/*
	if (pitchshift != 127) {
		if ((Between(status, 0x80, 0x8f) && (status != 0x89)) || (Between(status, 0x90, 0x9f) && (status != 0x99))) {
			if (pitchshiftchan[status - 0x80] == 1 || pitchshiftchan[status - 0x90] == 1) {
				int newnote = (note - 127) + pitchshift;
				if (newnote > 127) { newnote = 127; }
				else if (newnote < 0) { newnote = 0; }
				SETNOTE(dwParam1, newnote);
			}
		}
	}
	if (fullvelocity == 1 && (Between(status, 0x90, 0x9f) && velocity != 0))
		SETVELOCITY(dwParam1, 127);

	Understandable version of what the following function does
	*/

	if (ManagedSettings.TransposeValue != 0x7F)
	{
		if (!((dwParam1 - 0x80) & 0xE0))
		{
			if (pitchshiftchan[dwParam1 & 0xF])
			{
				int newnote = (((dwParam1 >> 8) & 0xFF) - 0x7F) + ManagedSettings.TransposeValue;
				if (newnote > 0x7F) { newnote = 0x7F; }
				else if (newnote < 0) { newnote = 0; }
				SETNOTE(dwParam1, newnote);
			}
		}
	}
	if (ManagedSettings.FullVelocityMode && (((dwParam1 & 0xFF) & 0xF0) == 0x90 && ((dwParam1 >> 16) & 0xFF)))
		SETVELOCITY(dwParam1, 0x7F);

	return dwParam1;
}

void __inline SendShortMIDIFeedback(DWORD dwParam1) {
	MMmidiOutShortMsg((HMIDIOUT)OMFeedback, dwParam1);
}

void __inline SendLongMIDIFeedback(LPMIDIHDR mHDR, UINT Size) {
	if (mHDR && Size != 0) {
		LPMIDIHDR lpMidiHdr = (MIDIHDR*)calloc(Size, sizeof(MIDIHDR));
		memcpy(lpMidiHdr, mHDR, Size);
		MMmidiOutLongMsg((HMIDIOUT)OMFeedback, lpMidiHdr, Size);
		free(lpMidiHdr);
	}
}

bool __inline SendToBASSMIDI(unsigned int ev) {
	/*
	
	For more info about how an event is structured, read this doc from Microsoft:
	https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-midioutshortmsg

	-
	TL;DR is here though:
	Let's assume that we have an event coming through, of value 0x007F7F95.
	MIDI events are ALWAYS big endian.

	The high-order byte of the high word is ignored.

	The low-order of the high word contains the first part of the data, which, for
	a NoteOn event, is the key number (from 0 to 127).

	The high-order of the low word contains the second part of the data, which, for
	a NoteOn event, is the velocity of the key (again, from 0 to 127).
	
	The low-order byte of the low word contains two nibbles combined into one.
	The first nibble contains the type of event (1001 is 0x9, which is a NoteOn),
	while the second nibble contains the channel (0101 is 0x5, which is the 5th channel).

	So, in short, we can read it as follows:
	0x957F7F should press the 128th key on the keyboard at full velocity, on MIDI channel 5.

	But hey! We can also receive events with a long running status, which means there's no status byte!
	That event, if we consider the same example from before, will look like this: 0x00007F7F

	Such event will only work if the driver receives a status event, which will be stored in memory.
	So, in a sequence of data like this:
	..
	1. 0x00044A90 (NoteOn on key 75 at velocity 10)
	2. 0x00007F7F (... then on key 128 at velocity 127)
	3. 0x00006031 (... then on key 50 at velocity 96)
	4. 0x0000605F (... and finally on key 95 at velocity 96)
	..
	The same status from event 1 will be applied to all the status-less events from 2 and onwards.
	-

	INFO: ev will be recasted as char in some parts of the code, since those parts
	do not require the high-word part of the unsigned int.

	*/

	unsigned int tev = ev;

	if (CHKLRS(GETSTATUS(tev)) != 0) LastRunningStatus = GETSTATUS(tev);
	else tev = ev << 8 | LastRunningStatus;

	unsigned int ch = tev & 0xF;
	unsigned int evt = 0;
	unsigned int param = GETFP(tev);

	unsigned int cmd = GETCMD(tev);
	unsigned int len = 3;
	bool ok = true;

	switch (LastRunningStatus) {
	// Handle 0xFF (GS reset) first!
	case 0xFF:
		for (int i = 0; i < 16; i++) {
			if (!BASS_MIDI_StreamEvent(OMStream, i, MIDI_EVENT_SOUNDOFF, 0))
				ok = false;

			if (!BASS_MIDI_StreamEvent(OMStream, i, MIDI_EVENT_NOTESOFF, 0))
				ok = false;

			if (!BASS_MIDI_StreamEvent(OMStream, i, MIDI_EVENT_SYSTEM, MIDI_SYSTEM_DEFAULT))
				ok = false;
		}
		return ok;

	default:
		switch (GETCMD(ev)) {
		case MIDI_NOTEON:
			evt = MIDI_EVENT_NOTE;
			break;
		case MIDI_NOTEOFF:
			evt = MIDI_EVENT_NOTE;
			param = (char)param;
			break;
		case MIDI_POLYAFTER:
			evt = MIDI_EVENT_KEYPRES;
			break;
		case MIDI_PROGCHAN:
			evt = MIDI_EVENT_PROGRAM;
			param = (char)param;
			break;
		case MIDI_CHANAFTER:
			evt = MIDI_EVENT_CHANPRES;
			param = (char)param;
			break;
		default:
			// Some events do not have a specific counter part on BASSMIDI's side, so they have
			// to be directly fed to the library by using the BASS_MIDI_EVENTS_RAW flag.
			if (!(ev - 0x80 & 0xC0))
				return _BMSEs(OMStream, BASS_MIDI_EVENTS_RAW, &tev, 3);

			if (!((ev - 0xC0) & 0xE0)) len = 2;
			else if (GETCMD(ev) == 0xF0)
			{
				switch (ev & 0xF)
				{
				case 3:
					len = 2;
					break;
				default:
					// Not supported by OmniMIDI!
					return false;
				}
			}

			return _BMSEs(OMStream, BASS_MIDI_EVENTS_RAW, &tev, len);
		}
	}

	return _BMSE(OMStream, ch, evt, param);
}

void __inline PrepareForBASSMIDI(DWORD dwParam1) {
	BASS_MIDI_EVENT Evs[2];

	if (ManagedSettings.FullVelocityMode || ManagedSettings.TransposeValue != 0x7F)
		dwParam1 = ReturnEditedEvent(dwParam1);

	_FeedbackShortMsg(dwParam1);

	if (ManagedSettings.OverrideNoteLength || ManagedSettings.DelayNoteOff) {
		if (((dwParam1 & 0xF0) == MIDI_NOTEON && ((dwParam1 >> 16) & 0xFF))) {

			Evs[0] = { MIDI_EVENT_NOTE, dwParam1 >> 8, dwParam1 & 0xF, 0, 0 };

			if (ManagedSettings.OverrideNoteLength)
				Evs[1] = { MIDI_EVENT_NOTE, (BYTE)(dwParam1 >> 8), dwParam1 & 0xF, FNoteLengthValue, 0 };

			_BMSEs(OMStream, BMSEsFlags, &Evs, ManagedSettings.OverrideNoteLength ? 2 : 1);

			return;
		}
		else if ((dwParam1 & 0xF0) == MIDI_NOTEOFF) {
			if (!ManagedSettings.OverrideNoteLength && ManagedSettings.DelayNoteOff) {
				Evs[0] = { MIDI_EVENT_NOTE, (BYTE)(dwParam1 >> 8), dwParam1 & 0xF, FDelayNoteOff, 0 };

				_BMSEs(OMStream, BMSEsFlags, &Evs, 1);
			}

			return;
		}
	}

	SendToBASSMIDI(dwParam1);
}

void __inline PrepareForBASSMIDIHyper(DWORD dwParam1) {
	SendToBASSMIDI(dwParam1);
}

void __inline SendLongToBASSMIDI(LPMIDIHDR IIMidiHdr) {
	// If dwBytesRecorded is 0, use dwBufferLength instead
	// Thank you MSDN for not telling me about it, 
	// and thanks to Windows Media Player for doing this...
	DWORD FLen = IIMidiHdr->dwBytesRecorded < 1 ? IIMidiHdr->dwBufferLength : IIMidiHdr->dwBytesRecorded;

	_BMSEs(OMStream, BASS_MIDI_EVENTS_RAW, IIMidiHdr->lpData, FLen);
	PrintLongMessageToDebugLog(IIMidiHdr);
}

// PBufData and PBufDataHyper have been merged,
// since Running Status is a MANDATORY feature in MIDI drivers
// and can not be skipped
void __inline PBufData(void) {
	DWORD dwParam1 = EVBuffer.Buffer[EVBuffer.ReadHead].Event;

	if (++EVBuffer.ReadHead >= EVBuffer.BufSize) EVBuffer.ReadHead = 0;

	_PforBASSMIDI(dwParam1);
}

void __inline PSmallBufData(void)
{
	auto HeadStart = EVBuffer.ReadHead;
	auto HeadPos = HeadStart; //fast volatile
	DWORD dwParam1 = EVBuffer.Buffer[HeadPos].Event;

	if (!~dwParam1) return;

	for (;;)
	{
		EVBuffer.Buffer[HeadPos].Event = ~0;

		if (++HeadPos >= EVBuffer.BufSize)
			HeadPos = 0;

		EVBuffer.ReadHead = HeadPos;

		if (~dwParam1) _PforBASSMIDI(dwParam1);
		else return;

		if (HeadPos == HeadStart) return;

		dwParam1 = EVBuffer.Buffer[HeadPos].Event;
		if (!~dwParam1) return;
	}
}

void __inline PlayBufferedData(void) {
	if (ManagedSettings.IgnoreAllEvents)
		return;
	
	if (EVBuffer.BufSize >= SMALLBUFFER) {
		if (!BufferCheck()) {
			_FWAIT;
			return;
		}

		do PBufData();
		while (BufferCheck());
	}
	else PSmallBufData();
}

void __inline PlayBufferedDataHyper(void) {
	if (!BufferCheck()) {
		_FWAIT;
		return;
	}

	do PBufData();
	while (EVBuffer.ReadHead != EVBuffer.WriteHeadCached);
	//while (EVBuffer.ReadHead != EVBuffer.WriteHead);
}

void __inline PlayBufferedDataChunk(void) {
	if (ManagedSettings.IgnoreAllEvents || !BufferCheck()) return;

	if (EVBuffer.BufSize >= SMALLBUFFER)
	{
		ULONGLONG whe = EVBuffer.WriteHead;
		do PBufData();
		while (EVBuffer.ReadHead != whe);
	}
	else PSmallBufData();
}

void __inline PlayBufferedDataChunkHyper(void) {
	if (!BufferCheck()) return;

	ULONGLONG whe = EVBuffer.WriteHead;
	do PBufData();
	while (EVBuffer.ReadHead != whe);
}

void __inline ParseData(DWORD_PTR dwParam1) {
	// Some checks
	if (CheckIfEventIsToIgnore(dwParam1) || !EVBuffer.Buffer)
		return;

	// Prepare the event in the buffer
	// LockForWriting(&EPThreadsL);

	auto NextWriteHead = EVBuffer.WriteHead + 1;
	if (NextWriteHead >= EVBuffer.BufSize) NextWriteHead = 0;

	if (NextWriteHead != EVBuffer.ReadHead)
	{
		EVBuffer.Buffer[EVBuffer.WriteHead].Event = dwParam1;
		EVBuffer.WriteHead = NextWriteHead;
	}
	else if (!ManagedSettings.DontMissNotes)
	{
		EVBuffer.Buffer[EVBuffer.WriteHead].Event = dwParam1;
		//do NOT advance the WriteHead for a more sensical note skipping
		//EVBuffer.WriteHead = NextWriteHead;
	}
	else
	{
		if (NextWriteHead >= EVBuffer.BufSize)
			NextWriteHead = EVBuffer.ReadHead; //guaranteed to be always in bounds

		if(EVBuffer.BufSize >= SMALLBUFFER)
			while (NextWriteHead == EVBuffer.ReadHead) _FWAIT;

		EVBuffer.Buffer[EVBuffer.WriteHead].Event = dwParam1;
		EVBuffer.WriteHead = NextWriteHead;
	}

	PrintEventToDebugLog(dwParam1);

	// UnlockForWriting(&EPThreadsL);
}

void __inline ParseDataHyper(DWORD_PTR dwParam1)
{
	// LockForWriting(&EPThreadsL);

	ULONGLONG NextWriteHead = EVBuffer.WriteHead + 1;
	if (NextWriteHead >= EVBuffer.BufSize) NextWriteHead = 0;

	EVBuffer.Buffer[EVBuffer.WriteHead].Event = dwParam1;

	EVBuffer.EventsSent++;
	/*
	if (NextWriteHead != EVBuffer.ReadHead)
		EVBuffer.WriteHead = NextWriteHead; //skip notes properly
	else
		EVBuffer.EventsSkipped++;
	*/
	if (NextWriteHead == EVBuffer.ReadHeadCached) {
		EVBuffer.ReadHeadCached = EVBuffer.ReadHead;
		if (NextWriteHead == EVBuffer.ReadHeadCached) {
			EVBuffer.EventsSkipped++;
			return;
		}
	}
	EVBuffer.WriteHead = NextWriteHead;

	// UnlockForWriting(&EPThreadsL);
}