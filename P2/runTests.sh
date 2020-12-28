#!/bin/bash

#Verificacao dos argumentos
if [[ "$#" -ne 3 ]] || [[ $3 -lt 1 ]] || [[ $3 -gt 100 ]]
then
    echo ERROR: Invalid format.
    echo Use ./runTests.sh inputDirectory outputDirectory maxThreads
    echo maxThreads must be a number between 1 and 100
    exit 0
fi

if [[ ! -d "$1" ]]
then
    echo ERROR: Folder '"'$1'"' does not exist.
    exit 0
fi


inputDir=$1
outputDir=$2
maxThreads=$3

#Cria directoria e subdirectorias, caso nao existam
mkdir -p $outputDir

for inputfile in "$inputDir"/*.txt
do
    for threads in $(seq 1 $maxThreads)
    do
        #extrai o ficheiro do path
        file=${inputfile##*/}

        #remove a extencao do ficheiro (ultimos 4 caracteres, .txt)
        #e concatena com o numero de threads e a extencao novamente
        outputFile=${file:0:${#file}-4}"-${threads}.txt"

        echo "InputFile="$file" NumThreads="${threads}

        #grep filtra o output apenas para a linha que contem a string escolhida
        ./tecnicofs $inputDir"/"$file $outputDir"/"$outputFile $threads | grep "TecnicoFS completed in"
    done
done
