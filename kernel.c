#include "frame_buffer.h"

static char *fb = (char *)FB_BASE_ADDRESS; // frameBuffer address

int currLine = 8; // current line we are at
int currPos = 0; // current column

char folderNames[50][10]; // index of folder names
int folderChildren[50][12]; // [i][0] is parent.  [i][2]-[11] is children
int folderNum = 0; // total number of folders
int currDir = 0; // index of directory we are in


char state = 'c'; //c: command o: object
char cmd[15] = "               "; //inputted command
int cmdIndex = 0; //current char index in command
char obj[15] = "               "; //inputted object
int objIndex = 0; //current char index in object




char cmdInput[20] = "                    "; // inputted string
int inputIndex = 0; // index of current char


void fb_move_cursor(unsigned short pos) { // move the cursor in the fb
    currPos = pos - currLine *80; // set currpos to cursor location
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT, ((pos >> 8) & 0x00FF));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT, pos & 0x00FF);
}

void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg){  // write char to fb
    i = i + (currLine * 80);  // calculate position
    fb[i*2] = c;  // set character
    fb[i*2 + 1] = ((fg & 0x0F) << 4) | (bg & 0x0F); // set background and char color
}

int fb_write(char *buf, unsigned int len) { //write string to fb
    unsigned int indexToBuffer = 0;
    while (indexToBuffer < len) {
        fb_write_cell(currPos, buf[indexToBuffer], COLOR_BLACK, COLOR_LIGHT_GREY);
        fb_move_cursor(currLine * 80 + currPos+1);
        indexToBuffer++;
    }
    return 0;
}

#define KEYBOARD_DATA_PORT 0x60  // locate keyboard port


unsigned char getCode(void)
{
	return inb(KEYBOARD_DATA_PORT); // get keyboard code
}

unsigned char getAscii(unsigned char scan_code) // convert code to ascii using array (credit: Malshani Dahanayaka)
{
	unsigned char ascii[256] =
	{
		'n', '0', '1', '2', '3', '4', '5', '6',		// 0 - 7
		'7', '8', '9', '0', '-', '=', 0x0, 0x0,		// 8 - 15
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',		// 16 - 23
		'o', 'p', '[', ']', '\n', 0x0, 'a', 's',	// 24 - 31
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',		// 32 - 39
		'\'', '`', 0x0, '\\', 'z', 'x', 'c', 'v',	// 40 - 47
		'b', 'n', 'm', ',', '.', '/', 0x0, '*',		// 48 - 55
		0x0, ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,		// 56 - 63
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '7',		// 64 - 71
		'8', '9', '-', '4', '5', '6', '+', '1',		// 72 - 79
		'2', '3', '0', '.'				// 80 - 83
	};

	return ascii[scan_code];
}

void newLine(void) { // move down one row
    currLine++;
    fb_move_cursor(currLine * 80);
}

int compareStr(char *input, int inLength, char *reference, int refLength) { // compare two strings
    if(inLength != refLength) { // if lengths are not equal, return 0
        return 0;
    }
    int i = 0;

    while(i < inLength) { // if all chars are not the same, return 0
        if(input[i] != reference[i]) {
            return 0;
        }
        i++;
    }

    return 1;
}

int compareNames(int indexOne, char * two) { // compare folder name against string
    int index = 0;
    while(folderNames[indexOne][index] != ' ') {
        if(folderNames[indexOne][index] != two[index]) {
            return 0;
        }
        index++;
    }
    return 1;
}

void resetStr(char *str, int length) { // reset a string to be blank
    int index = 0;
    while(index < length) {
        str[index] = ' ';
        index++;
    }
}

void clearLine() { // overwrite a row to be blank
    char blankBuf[80];
    for(int i = 0; i < 80; i++) {
        blankBuf[i] = ' ';
    }
    fb_write(blankBuf, 80);
}

void clearFiles() { // delete all files, reseting their indexes and names
    for(int i = 0; i < 50; i++) {
        for(int b = 0; b < 12; b++) {
            folderChildren[i][b] = -1; // -1 means null/no child/parent
        }
        for(int b = 0; b < 10; b++) {
            folderNames[i][b] = ' ';
        }
    }
}

void writeFolderName(char list[50][10], int index) { // print a folders name to fb
    int length = 0;
    char bname[11] = "          ";
    while(list[index][length] != ' '){
        length++;
    }
    for(int i = 0; i < length; i++) {
        bname[i] = list[index][i];
    }
    bname[length] = '/';
    fb_write(bname, length+1);
}

void exeDir() { // print all children folders of currDir to screen
    int i = 2;
    while(i < 12 && folderChildren[currDir][i] > -1) { // while child index != -1
        writeFolderName(folderNames, folderChildren[currDir][i]);
        i++;
        newLine();
    }
}

void clearSpace() { // clear the info space below command line
    currLine = 9;
    fb_move_cursor(currLine * 80);
    clearLine();
    clearLine();
    clearLine();
    clearLine();
    currLine = 8;
    fb_move_cursor(currLine * 80);
}

void writeDir(int index) { // print current folder and all parent folders to screen
    if(index != 0) {
        writeDir(folderChildren[index][0]);
    }
    writeFolderName(folderNames, index);
}

void createFolder(char name[10], int parent, int length) { // create a new folder
    if(length > 0 && length < 11) {
        folderNum++;
        int childIndex = 0;
        for(int i = 2; i < 12; i++) { // find next available child slot
            if(folderChildren[parent][i] == -1) {
                childIndex = i; // insert child index at slot
                i = 12;
            }
        }
        folderChildren[parent][childIndex] = folderNum;
        folderChildren[folderNum][0] = parent;
        for(int i = 0; i < length; i++) {
            folderNames[folderNum][i] = name[i]; // set folder name
        }
    currDir = folderNum; // move us to folder
    }
}

void goToParent() { // return to parent folder
    if(currDir > 0) {
        currDir = folderChildren[currDir][0];
    }
}

void goToChild(char *child) { // go to child folder
    int index = 2;
    while(folderChildren[currDir][index] != -1 && index < 12) {
        if(compareNames(folderChildren[currDir][index], child) == 1) { // compare name inputted to list of children
            currDir = folderChildren[currDir][index]; // move to child
        }
        index++;
    }
}

void deleteIndex(int index) {  // delete folder at index
    for(int i = 2; i < 12; i++) {
        if(folderChildren[index][i] > 0) {
            deleteIndex(folderChildren[index][i]); // recursively delete children of folder
            folderChildren[index][i] = -1; // set to "null"
        }
    }
}

void delete(char *name) { // delete folder from name
    int index = 2;
    while(folderChildren[currDir][index] != -1 && index < 12) { // compare input against all children
        if(compareNames(folderChildren[currDir][index], name) == 1) {
            deleteIndex(folderChildren[currDir][index]);
            for(int i = index; i < 11; i++) {
                folderChildren[currDir][i] = folderChildren[currDir][i + 1]; // remove deleted folder from child list
            }
            index--;
        }
        index++;
    }
}

void parseCommand() { // segment input into command and object
    state = 'c';
    objIndex = 0;
    cmdIndex = 0;
    for(int i = 0; i < 20; i++) {
        if(state == 'c') {
            if(cmdInput[i] != ' ') { // command is anything before space
                cmd[cmdIndex] = cmdInput[i];
                cmdIndex++;
            } else { // object is anything after space
                state = 'o';
            }
        } else if(state == 'o') {
            if(cmdInput[i] != ' ') {
                obj[objIndex] = cmdInput[i];
                objIndex++;
            } else {
                i = 20;
            }

        }
    }
}

void exe(char *cmd, int cmdLength, char *obj, int objLength) { // execute command
    char new[3] = "new";
    char cd[2] = "cd";
    char del[6] = "delete";
    char dir[3] = "dir";
    char invalid[16] = "invalid command ";

    if (compareStr(cmd, cmdLength, new, 3) == 1) { // if the command is "new"
        createFolder(obj, currDir, objLength); // create folder with the name in char array obj
    } else if (compareStr(cmd, cmdLength, cd, 2) == 1) { // if "cd"
        char up[1] = ".";

        if(compareStr(obj, objLength, up, 1) == 1){ // if obj is "."
            goToParent();
        } else {
            goToChild(obj); // else go to child with name in obj
        }
    } else if (compareStr(cmd, cmdLength, del, 6) == 1) { // if "del"
        delete(obj);
    } else if (compareStr(cmd, cmdLength, dir, 3) == 1) { // if "dir"
        newLine();
        exeDir();
    } else { // if command not recognized
        newLine();
        newLine();
        fb_write(invalid, 16);
    }
}

int kernel_main() // kernel begins here
    {
        clearFiles(); // clear all folders
        char homeD[4] = "home";
        for(int i = 0; i < 4; i++) {
            folderNames[0][i] = homeD[i]; // create folder "home"
        }
        writeDir(currDir); // print directory to screen

        char endchar = '0'; // arbitrary values to begin loop
        char oldCode = 0;
        char newCode = 0;

        while(newCode != 1) { // esc kills loop
            newCode = getCode(); // get input code
            endchar = getAscii(newCode); // convert to char
            if(newCode == 28 && newCode != oldCode) { // if enter

                newLine();
                parseCommand(); // segment input
                clearSpace(); // clear info
                exe(cmd, cmdIndex, obj, objIndex); // execute input
                currLine = 8;
                fb_move_cursor(currLine * 80); // reset cursor to input row
                clearLine(); // clear it
                currLine = 7;
                writeDir(currDir); // print current location
                resetStr(cmd, 15); // reset all strings and index
                resetStr(obj, 15);
                resetStr(cmdInput, 20);
                inputIndex = 0;
                
            } else if (newCode == 14 && newCode != oldCode) { // if backspace
                if(inputIndex > 0) {
                    fb_write_cell(currPos - 1, endchar, COLOR_BLACK, COLOR_LIGHT_GREY); // print " "
                    fb_move_cursor(currLine * 80 + currPos - 1); // move back cursor
                    inputIndex--; // move back index
                    cmdInput[inputIndex] = ' '; // write " " to input array
                }
            } else if (endchar != 0x0 && newCode != oldCode) { // else (should be a char inputted)
                fb_write_cell(currPos, endchar, COLOR_BLACK, COLOR_LIGHT_GREY); // print inputed char
                fb_move_cursor(currLine * 80 + currPos + 1); // move cursor forward
                cmdInput[inputIndex] = endchar; // write to input array
                inputIndex++; // move index forward
            }
            oldCode = newCode; // prevent loop from executing same character again
        }

        return 1;
    }