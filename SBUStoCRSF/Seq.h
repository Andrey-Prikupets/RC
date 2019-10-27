#pragma once

#include <stdint.h>
#include "debug.h"
#include "config.h"

typedef uint8_t Seq10ms;

class Seq {
public:
	Seq10ms* data;
	uint8_t size;

	Seq (Seq10ms* aData, uint8_t aSize) : data(aData), size(aSize) {}
};

#define NEW_SEQ(name,...) static Seq10ms name##_ARR[] = {__VA_ARGS__}; Seq name (name##_ARR, sizeof(name##_ARR)/sizeof(Seq10ms));
#define BEEP_MS(delay) ((delay)/10)
#define PAUSE_MS(delay) ((delay)/10)

class Beeper {
public:
	void play(Seq* seq);
	void stop(Seq* seq);
	void play(uint8_t seqIndex);
	void stop(uint8_t seqIndex);
	Beeper(uint8_t aPin, bool aPinIsInverted, Seq10ms aPause10Ms, uint8_t aQueueSize, Seq* aSeqs[], uint8_t aNumSeqs);
	void loop(void);
private:
	Seq** seqs;
	uint8_t numSeqs;
	uint8_t step;
	bool stepIsBeep;
	bool isBeeping;
	bool isInPause;
	uint16_t pauseMs;
	uint8_t pin;
	bool pinIsInverted;
	unsigned long nextTimeMs;
	uint8_t* queue;
	uint8_t queueSize;
	uint8_t queueCount;

	void start(uint8_t seqIndex);
	void beep(bool enable);

#define NOT_FOUND 0xFF
	void initQueue(uint8_t size);
	uint8_t findInQueue(uint8_t startPos, uint8_t seqIndex);
	void appendToQueue(uint8_t seqIndex);
	void removeFromQueue(uint8_t position);
	uint8_t findSeq(Seq* seq);
	void removeIfNotPlayed(uint8_t seqIndex);
};
