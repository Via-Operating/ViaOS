# DON'T DELETE THIS! THIS IS AN IMPORTANT FILE!
HEADER=commands.h
echo -n "" > $HEADER
echo "// DON'T DELETE THIS FILE!" >> $HEADER
echo "#ifndef __ALL_HEADERS__" >> $HEADER
echo "#define __ALL_HEADERS__" >> $HEADER
for file in term/viaSh/*.h
do
    echo "#include <$file>" >> $HEADER
done
echo "#endif" >> $HEADER