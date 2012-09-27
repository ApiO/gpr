if exist gpr\include rd /s /q gpr\include
if exist gpr\src     rd /s /q gpr\src

mklink /j gpr\include ..\..\include
mklink /j gpr\src     ..\..\src