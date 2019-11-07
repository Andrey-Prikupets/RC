#include "Seq.h"

Beeper::Beeper(uint8_t aPin, bool aPinIsInverted, Seq10ms aPause10Ms, uint8_t aQueueSize, Seq* aSeqs[], uint8_t aNumSeqs) :
 pin(aPin), pinIsInverted(aPinIsInverted), pauseMs(((uint16_t)aPause10Ms)*10), seqs(aSeqs), numSeqs(aNumSeqs), 
 step(0), isBeeping(false), isInPause(false) {
	pinMode(pin, OUTPUT);
  digitalWrite(pin, pinIsInverted != isBeeping);
	initQueue(aQueueSize);
}

void Beeper::initQueue(uint8_t size) {
	queueSize = size;
	queue = new uint8_t[queueSize];
	queueCount = 0;
}

uint8_t Beeper::findInQueue(uint8_t startPos, uint8_t seqIndex) {
	for (uint8_t i=startPos; i<queueCount; i++) {
		if (queue[i] == seqIndex)
			return i;
	}
	return NOT_FOUND;
}

void Beeper::appendToQueue(uint8_t seqIndex) {
	if (queueCount >= queueSize) {
#ifdef SEQ_DEBUG
		Serial.print ("Overflow seqIndex "); Serial.println(seqIndex, DEC);
#endif
		return;
	}
	queue[queueCount] = seqIndex;
	queueCount++;
}

void Beeper::removeFromQueue(uint8_t position) {
	if (queueCount == 0 || position+1 > queueCount) {
#ifdef SEQ_DEBUG
    Serial.print ("Underflow position "); Serial.println(position, DEC);
#endif
		return;
	}
	memmove(queue+position, queue+position+1, queueCount-position-1);
	queueCount--;
}

void Beeper::beep(bool enable) {
#ifdef SEQ_DEBUG
    Serial.print(millis(), DEC); Serial.println (enable ? "; BEEP" : "; OFF");
#endif
#ifdef SOUND_OFF  
  isBeeping = false;
#else
  isBeeping = enable;
#endif
	digitalWrite(pin, pinIsInverted != isBeeping);
}

void Beeper::removeIfNotPlayed(uint8_t seqIndex) {
	// Search given sequence in the queue;
	// If it is in the queue and not played, remove it from all places and add it to the end;
	// If it is in the queue and played, remove it from all places except 0 and add it to the end;
	// If it is not in the queue, add it to the end;
	uint8_t firstPos = isInPause ? 0 : 1;
	uint8_t pos = findInQueue(firstPos, seqIndex);
	// Delete all seqIndex entries (except 0 if it is already played);
	for (; pos != NOT_FOUND; pos = findInQueue(firstPos, seqIndex)) {
		removeFromQueue(pos);
	}
}

void Beeper::loop(void) {
	unsigned long ms = millis();
	if (isInPause) {
		if (ms < nextTimeMs) // Note: millis() rollover in 49 days;
			return;
       	isInPause = false;

       	// Select next sequence and play without lead-in;
       	if (queueCount == 0)
       		return;

		// Start playing 0 sequence;
		// Note: Assuming here there are no sequences with 0 in the beginning;
       	step = 0;
       	uint8_t seqIndex = queue[0];
		Seq* seq = seqs[seqIndex];
       	uint16_t delayMs = ((uint16_t)seq->data[step])*10;
#ifdef SEQ_DEBUG
    Serial.print(ms, DEC); 
    Serial.print ("; Beeper::loop, start play seq "); Serial.print(seqIndex , DEC);
    Serial.print (" after pause - step "); Serial.print(step , DEC);
    Serial.print (" delay "); Serial.println(delayMs , DEC);
#endif
       	stepIsBeep = true;
       	nextTimeMs = ms+delayMs;
		step++;
		beep(stepIsBeep);
	} else {
		if (queueCount == 0)
			return;		
		if (ms < nextTimeMs) // Note: millis() rollover in 49 days;
			return;
       	uint8_t seqIndex = queue[0];
		Seq* seq = seqs[seqIndex];
		if (step < seq->size) {
       		uint16_t delayMs = ((uint16_t)seq->data[step])*10;
#ifdef SEQ_DEBUG
    Serial.print(ms, DEC); 
    Serial.print ("; Beeper::loop, continue playing seq "); Serial.print(seqIndex , DEC);
    Serial.print (" after pause - step "); Serial.print(step , DEC);
    Serial.print (" delay "); Serial.println(delayMs , DEC);
#endif
			step++;
	   		stepIsBeep = !stepIsBeep;
   			nextTimeMs = ms+delayMs;			
			beep(stepIsBeep);
		} else {
#ifdef SEQ_DEBUG
    Serial.print(ms, DEC); 
    Serial.print ("; Beeper::loop, finish playing seq "); Serial.print(seqIndex , DEC);
    Serial.print (" - step "); Serial.print(step , DEC);
    Serial.print (" starting pause for "); Serial.println(pauseMs , DEC);
#endif
			removeFromQueue(0); // Remove finished sequence from the queue;
			// Initiate lead-out pause;
			isInPause = true;
       		nextTimeMs = ms+pauseMs;			
			step = 0;
			stepIsBeep = false;
			beep(stepIsBeep);
		}
	}	
}

uint8_t Beeper::findSeq(Seq* seq) {
	for (uint8_t i=0; i<numSeqs; i++) {
		if (seqs[i] == seq)
			return i;
	}
	return NOT_FOUND;
}

void Beeper::play(Seq* seq) {
	uint8_t seqIndex = findSeq(seq);
	if (seqIndex == NOT_FOUND) {
#ifdef SEQ_DEBUG
    Serial.println ("Play: seq not found "); 
#endif
		return;
	}
	play(seqIndex);
}

void Beeper::play(uint8_t seqIndex) {
	start(seqIndex);
}

void Beeper::stop(uint8_t seqIndex) {
	removeIfNotPlayed(seqIndex);
}

void Beeper::start(uint8_t seqIndex) {
	Seq* seq = seqs[seqIndex];
	// if (findInQueue(0, seqIndex) != NOT_FOUND)
	//	return;
	removeIfNotPlayed(seqIndex);
	appendToQueue(seqIndex);
}

void Beeper::stop(Seq* seq) {
	uint8_t seqIndex = findSeq(seq);
	if (seqIndex == NOT_FOUND) {
#ifdef SEQ_DEBUG
    Serial.println ("stop: seq not found "); 
#endif
		return;
	}
	stop(seqIndex);
}
