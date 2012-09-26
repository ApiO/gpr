if exist gpcore\include rd /s /q gpcore\include
if exist gpcore\src     rd /s /q gpcore\src

mklink /j gpcore\include ..\..\include
mklink /j gpcore\src     ..\..\src