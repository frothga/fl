#ifndef fl_color_h
#define fl_color_h


// Note: These #defines conflict with the X header files, and probably others
// as well.  Maybe a better strategy would be to make all the color constants
// into global variables, or to make them ALLCAPS, as is traditional for
// preprocessor contants.


#define white     0xFFFFFFFF
#define gray100   0xFFFFFFFF
#define gray50    0xFF808080
#define gray0     0xFF000000
#define black     0xFF000000

#define red       0xFFFF0000
#define green     0xFF00FF00
#define blue      0xFF0000FF

#define purple    0xFFFF00FF

#define cyan100   0xFF00FFFF
#define cyan50    0xFF008080
#define cyan      cyan100

#define yellow100 0xFFFFFF00
#define yellow90  0xFFE6E600
#define yellow75  0xFFC0C000
#define yellow50  0xFF808000
#define yellow    yellow90


#endif
