# Introduction

## What is Vibend
Vibend comes from bend and vivid. It means to bend the console to make it vivid. <br>
It is a c++ library inspired by ncurses.

## Vibend vs Ncurses
Vibend has the base functions as ncurses but there are vivids such as `Text Box`es and `Progress Bar`s and etc. We plan to add more vivid objects.

## Build or Install
You can build it your self or use our default builds for x86 windows Debug and Release. Vibend currently is not supported by vcpkg.

## How to Use
Every class and function in vibend splits into 2 groups:

### Benders
This group is mostly filled with functions.
These do the basics of this library. They are very similar to ncurses functions. They use ANSI Escape codes with functions such as `teleport(y, x)`, `printf(...)`, `Vibend::setforeground(color)` and etc.

### Vivids
This group is mostly filled with classes such as `Panel`,`TextBox`, `ProgressBar`, `ItemSelect` and etc.

We plan to add some example projects for vibend. Here is an example of how you can use it:
``` cpp
#include <vibend.h>

int main()
{
	using namespace Vibend::Literals;
	Vibend::init();
	Vibend::clear();

	Vibend::ProgressBar progressbar(
		Vibend::Box(25_vw, 50_vh - 1, 50_vw, 1),
		219, ' ', ' ',
		[]() {Vibend::setforeground(Vibend::Color::GREEN);},
		[]() {Vibend::resetforeground();}
	);

	Vibend::Panel(progressbar.box.outer()).drawborder();

	while (progressbar.progress < 1)
	{
		progressbar.progress += .01f;
		progressbar.draw();
		Sleep(10);
	}

	Vibend::getch();
	return 0;
}
```