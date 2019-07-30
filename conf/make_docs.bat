set DOCS_DIR=%1
set NOTEBOOK=%2
set DOXYGEN_EXEC=%3
set DOXYGEN_CONF=%4%

call %userprofile%\Anaconda3\condabin\activate.bat
jupyter nbconvert --to html --output %DOCS_DIR%/notebook.html %NOTEBOOK%
echo doxypypy -a -c %%1 > py_filter.bat
%DOXYGEN_EXEC% %DOXYGEN_CONF%
xcopy /Y/s/e html %DOCS_DIR%
latex/make.bat 
xcopy /Y latex/refman.pdf %DOCS_DIR%\\refman.pdf"
