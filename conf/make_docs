#!/bin/bash

DOCS_DIR="$1"
NOTEBOOK="$2"
DOXYGEN_EXEC="$3"
DOXYGEN_CONF="$4"

jupyter nbconvert --to html --output "${DOCS_DIR}/notebook.html" "${NOTEBOOK}"
echo -e "#!/bin/bash\ndoxypypy -a -c \"\$1\"" > py_filter
chmod +x py_filter
"${DOXYGEN_EXEC}" "${DOXYGEN_CONF}"
cp -r html/* "${DOCS_DIR}/"
make -C latex/
cp latex/refman.pdf "${DOCS_DIR}/refman.pdf"

