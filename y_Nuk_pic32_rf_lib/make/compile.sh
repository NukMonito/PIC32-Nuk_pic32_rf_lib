#!/bin/sh

#
# Sintax
# ../make/compile.sh mprocessor library directori
#
# where
#
#   mprocessor 
#     The name of the processor target ( like 32MX170F226B )
#





# -----------------------------------------------------------------------------
# Parameters
# $1 mprocessor
# $2 library directori
#
PROC=$1





# -----------------------------------------------------------------------------
# Own directori
#
DIR_NAME=$(pwd)




# -----------------------------------------------------------------------------
# Own includes
#
INC=" "
INC=$INC" "-Iinclude
INC=$INC" "-I../include
INC=$INC" "-I../../include





# -----------------------------------------------------------------------------
# Lib includes
#
cd ..
DIR_LIBS=$(pwd)
for LIB in $(ls -d *lib);
do
	INC=$INC" "-I$DIR_LIBS/$LIB/include
done




# -----------------------------------------------------------------------------
# Compiling and linking
#
cd $DIR_NAME
cd implementation
cd ../implementation

OPTC=-mprocessor=$PROC" "-O1

echo
#xc32-g++ -c $OPTC $INC -o  Nuk_pic32_rf.o  Nuk_pic32_rf.cpp
EXE="xc32-g++ -c "$OPTC" "$INC" -o  Nuk_pic32_rf.o  Nuk_pic32_rf.cpp"
echo $EXE
$EXE



#xc32-ar r Nuk_pic32_rf_lib.a Nuk_pic32_rf.o
EXE="xc32-ar r Nuk_pic32_rf_lib.a Nuk_pic32_rf.o"
echo $EXE
$EXE





# -----------------------------------------------------------------------------
# Move lib and clean *.o
#
mv *.a ../
rm *.o




# -----------------------------------------------------------------------------
# Return to the original directory
#
cd $DIR_NAME


