#!/usr/bin/python3

# Imports
from enum import Enum
from optparse import OptionParser
import fileinput
import os, sys
import re
import random
import shutil
import string
import tempfile
import uuid

########################################################################
# Print an error message and exit.
def error(errorMessage, exitValue=1):
    print("\nError:  " + errorMessage)
    progName = os.path.basename(sys.argv[0]) # could also use __file__
    print("\n(Use 'python " + progName + " -h' for help.)")
    sys.exit(exitValue)

########################################################################
# Like assert, but does not throw an exception (and dump stack).
def assertNice(test, errorMessage, exitValue=1):
    if not test:
        error(errorMessage, exitValue)

########################################################################
# Open a file.  Errors out if file can't be opened with specified options.
def mustOpen(filePath, openMode):
    assertNice(filePath != None and len(filePath) > 0, "must specify a filePath param.")
    assertNice(openMode != None and len(openMode) > 0, "must specify a openMode param.")
    file = None
    try:
        file = open(filePath, openMode)
    except BaseException as e:
        error("Couldn't open file [" + filePath + "] with open mode '" + openMode + "'")
    assertNice(file != None, "file is empty, shouldn't happen")
    return file

########################################################################
def generateRandomCharAndDigitSequence(
    size=6
):
    chars = string.ascii_letters + string.digits
    return ''.join(random.choice(chars) for _ in range(size))

########################################################################
def removeDestGroup(inLines, groupNameToRemove):
    outLines = []
    class State(Enum):
        NOT_FOUND = 1
        FOUND_GROUP = 2
        OUT_OF_GROUP = 3
    state = State.NOT_FOUND

    openRegex = re.compile('.*<GROUP .*name="'+groupNameToRemove+'">')
    closeRegex = re.compile('.*</GROUP>')

    for l in inLines:
        if state == State.NOT_FOUND:
            matchObj = openRegex.match(l)
            if not matchObj:
                outLines.append(l)
            else: # start skipping lines
                state = State.FOUND_GROUP
        elif state == State.FOUND_GROUP:
            matchObj = closeRegex.match(l)
            # don't output anything until it matches again
            if matchObj: 
                state = State.OUT_OF_GROUP
        else: # out of group
            outLines.append(l)

    return outLines

########################################################################
def getAndFixupSourceGroup(inLines, sourceGroupName, destGroupName):
    foundLines = []
    class State(Enum):
        NOT_FOUND = 1
        FOUND_GROUP = 2
        OUT_OF_GROUP = 3
    state = State.NOT_FOUND

    openRegex = re.compile('.*<GROUP .*name="'+sourceGroupName+'">')
    closeRegex = re.compile('.*</GROUP>')

    # extract the source group
    for l in inLines:
        if state == State.NOT_FOUND:
            matchObj = openRegex.match(l)
            if matchObj:
                foundLines.append(l)
                state = State.FOUND_GROUP
        elif state == State.FOUND_GROUP:
            matchObj = closeRegex.match(l)
            foundLines.append(l)
            if matchObj:
                state = State.OUT_OF_GROUP
                break

    # now update all the lines in that group to be for the destination
    checkLines = inLines.copy()
    upLines = []
    sourceRE = re.compile('.*('+sourceGroupName+').*')
    destUuidRE = re.compile(' *<GROUP id="(\{[0-9a-zA-Z\-]+)\}" name="'+destGroupName+'">')
    fileIdRE = re.compile(' *<FILE id="([a-zA-Z0-9]{6})" name=".*"')
    for l in foundLines:
        # replace source with dest
        matchObj = sourceRE.search(l)
        if matchObj:
            l = re.sub(matchObj.group(1), destGroupName, l)

        # change group id to be different uuid
        matchObj = destUuidRE.search(l)
        if matchObj:
            newUUID = str(uuid.uuid4()).upper()
            l = re.sub(matchObj.group(1), newUUID, l, count=1)

        # change file id to be new unique id
        matchObj = fileIdRE.search(l)
        if matchObj:
            isUnique = False
            while not isUnique:
                newId = generateRandomCharAndDigitSequence(6)
                idRE = re.compile('.*<FILE *id="'+newId+'"')
                for cl in checkLines:
                    match = idRE.search(cl)
                    if match:
                        break

                # if we got here there was no match, it's ok to use the id
                isUnique = True
                l = re.sub(matchObj.group(1), newId, l, count=1)

        upLines.append(l) 
        checkLines.append(l) # this ends up being the original lines plus any modified lines - important for checking for id uniqueness

    return upLines

########################################################################
def insertDestGroup(destGroupLines, inLines, sourceGroupName):
    outLines = []
    class State(Enum):
        NOT_FOUND = 1
        FOUND_GROUP = 2
    state = State.NOT_FOUND

    openRegex = re.compile('.*<GROUP .*name="'+sourceGroupName+'">')

    for l in inLines:
        if state == State.NOT_FOUND:
            matchObj = openRegex.match(l)
            if not matchObj:
                outLines.append(l)
            else: # found it.  insert the destGroupLines before this
                for d in destGroupLines:
                    outLines.append(d)
                outLines.append(l)
                state = State.FOUND_GROUP
        elif state == State.FOUND_GROUP:
            outLines.append(l)

    return outLines

########################################################################
def main():

    try:

        # Enforce minimum version.  
        EXPECTED_VERSION=(3,6)
        if sys.version_info < EXPECTED_VERSION:
            print("Must run on python " + str(EXPECTED_VERSION[0])+"."+str(EXPECTED_VERSION[1]) + " or greater.")
            print("Detected python version:  " + str(sys.version_info))
            sys.exit(1)

        # Parse the options and parameters.
        parser = OptionParser(
            usage='%prog -j  path/to/*.jucer',
            description='This script takes the "Source/audio_processing_float" dir, copies it to "Source/audio_processing_double", and updates the projucer file and headers to easily support double-precision audio buffers.',
            epilog='',
            version='%prog v0.1')
        parser.add_option(
            "-j", "--jucer-file", 
            action="store",
            dest="jucerFile",
            help="the path to the jucer file to update.  This file must exist.", 
            metavar="FILE",
            default="")
        (options, args) = parser.parse_args()

        if not options.jucerFile:
            error("Missing argument: -j")

        # dirs we need
        thisScriptAbs = os.path.abspath(__file__)
        #thisScriptAbs = os.path.abspath(".")
        thisScriptDir = os.path.dirname(thisScriptAbs)
        # the root of the source dir.  This script is in the 'bin' dir, off the root level.
        baseDir = os.path.dirname(thisScriptDir)
        singlePrecisionDir = baseDir + "/Source/audio_processing_float"
        doublePrecisionDir = baseDir + "/Source/audio_processing_double"

        # These are abstracted out so later we can add params in case we want to develop 
        # the double precision first, and copy it to single:
        # TODO support double -> single
        sourcePrecisionDir = singlePrecisionDir
        destPrecisionDir = doublePrecisionDir
        sourcePrecisionTypename = "float"
        destPrecisionTypename = "double"
        headerFile = destPrecisionDir + "/" + "audio_processing_header.h"
        jucerFile = os.path.abspath(options.jucerFile)

        print("\nUsing:")
        print("  baseDir            = "+baseDir)
        print("  singlePrecisionDir = "+singlePrecisionDir)
        print("  doublePrecisionDir = "+doublePrecisionDir)
        print("  sourcePrecisionDir = "+sourcePrecisionDir)
        print("  destPrecisionDir   = "+destPrecisionDir)
        print("  jucerFile          = "+jucerFile)
        print("")

        # check options
        if not os.path.isfile(jucerFile):
            error("could not find specified .jucer file:  "+jucerFile)

        # a. remove all files in the dest dir, and copy the source dir to dest dir
        shutil.rmtree(destPrecisionDir)
        shutil.copytree(sourcePrecisionDir, destPrecisionDir)

        #b. Edit the file audio_processing_double/audio_processing_header.h to:
        #    * Update #define for `SAMPLE_TYPE` to be `double`
        #    * Update #define for `AUDIO_PROCESSING_NAMESPACE` to be `double`
        #    * Change the header guard appropriately.
        # This all can be done with one search & replace:

        # make sure it exists
        if not os.path.isfile(headerFile): 
            error("couldn't find input header file: " + sourcePrecisionDir + "/" + headerFile ) # error out with the source because it was just copied from here

        # replace all (ie. float -> double or vice versa)
        with fileinput.FileInput(headerFile, inplace=True, backup='.bak') as file:
            for line in file:
                print(line.replace(sourcePrecisionTypename, destPrecisionTypename), end='')

        #c. Edit your "`*.jucer`" file to add the new files (it removes all existing from that dir first).
        # TODO it's best to use a parser like minidom, but for now just hack
        # it up with strings.  Who knows, maybe if it renders the xml back to a string
        # differently, juce won't be able to read it.  At least this way it will look 
        # the same / similar.
        #from xml.dom import minidom

        with open(jucerFile, 'r') as file:
            origLines = file.readlines()

        #print("origLines = "+str(origLines))
        #print("type(origLines) = "+str(type(origLines)))

        #    * Find the GROUP named "audio_processing_double", delete it
        destGroupName = "audio_processing_"+destPrecisionTypename
        sourceGroupName = "audio_processing_"+sourcePrecisionTypename
        noDestLines = removeDestGroup(origLines, destGroupName)
        #print("\n\n\nnoDestLines = \n\n")
        #for l in noDestLines:
        #    print(l, end='')

        #    * Find the GROUP named "audio_processing_float", copy it to "audio_processing_float".
        #    * Change the GROUP id to be a different UUID
        #    * Change the FILE ids to be new unique IDs.  (random 6-char [a-zA-Z0-9]{6} sequence, check by grepping the file again, if found, re-randomize)
        #    * Fix the paths to the files to be `Source/audio_processing_double/......`
        destGroup = getAndFixupSourceGroup(noDestLines, sourceGroupName, destGroupName)
        finalLines = insertDestGroup(destGroup, noDestLines, sourceGroupName)

        # copy the file to .bak (overwrites it if already exists)
        jucerFileBak = jucerFile + ".bak"
        shutil.copyfile(jucerFile, jucerFileBak)

        # Write the file out again
        with open(jucerFile, 'w') as file:
            file.writelines(finalLines)

        #    * Delete the "Builds" directory (works on Windows; not sure about MacOS/Linux/etc)
        buildsDir = baseDir + "/Builds"
        if os.path.isdir(buildsDir):
            shutil.rmtree(buildsDir)

    except Exception as e:
        raise e

    print("\nSuccess!")

    print("\nDone.\n")

# execute only if run as a script
if __name__ == "__main__":
    main()

