#ifndef QTSETFUNCTION_H
#define QTSETFUNCTION_H
extern unsigned int unCardIndex;
int QT_BoardGetBoardInfo();
int QT_BoardSetRecRepMode();
int QT_BoardSetFanCtrl();
int QT_BoardSetRecChSelect();
int QT_BoardSetAFE();
int QT_BoardSetRecClockMode();
int QT_BoardSetRecTrigMode(int workMode);

int QT_BoardSetRepChSelect();
int QT_BoardSetRepClockMode();
int QT_BoardSetRepTrigMode(int workMode);

int QT_BoardSetRecStdSingle(int blockNum);
int QT_BoardSetRecStdMulti(int blockNum);
int QT_BoardSetRecFIFOSingle(int blockNum);
int QT_BoardSetRecFIFOMulti(int blockNum);

int QT_BoardSetRepDDSPlay();
int QT_BoardSetRepFilePlay(int blockNum);

int QT_BoardSet9009RFParam(int workMode);
#endif