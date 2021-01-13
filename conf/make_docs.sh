#!/bin/bash

DOXYGEN_EXEC="$1"
DOXYGEN_CONF="$2"
INPUT_DIR="$3"
FILE_BASE="$4"
OUTPUT_DIR="$5"

emacs "$INPUT_DIR/$FILE_BASE.org" --batch -f org-html-export-to-html --kill
cp "$INPUT_DIR/$FILE_BASE.html" "$OUTPUT_DIR/$FILE_BASE.html" 

echo -e "#!/bin/bash\ndoxypypy -a -c \"\$1\"" > py_filter
chmod +x py_filter
"${DOXYGEN_EXEC}" "${DOXYGEN_CONF}"
cp -r html/* "${OUTPUT_DIR}/"
make -C latex/
cp latex/refman.pdf "${OUTPUT_DIR}/refman.pdf"

