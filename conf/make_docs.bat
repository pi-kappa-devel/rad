set DOXYGEN_EXEC=%1
set DOXYGEN_CONF=%2
set INPUT_DIR=%3
set FILE_BASE=%4
set OUTPUT_DIR=%5

emacs %INPUT_DIR%\\%FILE_BASE%.org --batch -f org-html-export-to-html --kill
xcopy /Y %INPUT_DIR%\\%FILE_BASE%.html %OUTPUT_DIR%\\%FILE_BASE%.html

echo doxypypy -a -c %%1 > py_filter.bat
%DOXYGEN_EXEC% %DOXYGEN_CONF%
xcopy /Y/s/e html %OUTPUT_DIR%
latex/make.bat 
xcopy /Y latex\\refman.pdf %OUTPUT_DIR%\\refman.pdf
