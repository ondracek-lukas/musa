// Spectrograph  Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

enum drawerScale {
	UNSCALED=0,
	LINEAR_SCALE,
	LOG_SCALE
};
extern void drawerAddColumns(float *buff, size_t columns, size_t drawingOffset); // column-oriented
extern void drawerSetInputColumnLen(size_t value, size_t toBeDrawen);
extern void drawerSetScale(enum drawerScale s, double minFrequency, double maxFrequency, double anchoredFrequency, bool tones, bool hideScaleLines, bool showKeyboard, bool coloredOvertones);
extern void drawerClearBuffer(size_t w, size_t h);
extern void drawerRepaint(bool force);
