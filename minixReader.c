/******
* 50% - Kyle Hoffman 2701222 
* 50% - Jonathan Marc 2664174
*
* This program takes several user entered commands to dissect and read files
* using the minix file system. The code can mount and unmount files, show the 
* superblock of a mounted file, traverse the files of the root directory in
* with minimal or maximum detail, show user specified data zones, and give
* a hex dump of a user specified file.
*
* CIS 340 - Project 4
*
* Group 4
*******/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>
#include <minixReader.h>

//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Creates a minix superblock structure
struct minix_super_block {
	unsigned short s_ninodes; 
	unsigned short s_nzones; 
	unsigned short s_imap_blocks; 
	unsigned short s_zmap_blocks; 
	unsigned short s_firstdatazone; 
	unsigned short s_log_zone_size; 
	unsigned int s_max_size; 
	unsigned short s_magic; 
	unsigned short s_state; 
	unsigned int s_zones; 
};
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Creates a minix inode structure
struct minix_inode {
	unsigned short i_mode; 
	unsigned short i_uid; 
	unsigned int i_size; 
	unsigned int i_time; 
	unsigned char i_gid; 
	unsigned char i_nlinks; 
	unsigned short i_zone[9]; 
};
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Creates a minix directory entry structure
struct minix_dir_entry {
	unsigned short inode; 
	char name[0]; 
};
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Sets mounted files to FALSE
// Global variable
int fd = 0;
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Prints out all of the possible commands the user many enter 
void help() {
	printf("\n\thelp");
	printf("\n\tminimount");
	printf("\n\tminiumount");
	printf("\n\tshowSuper");
	printf("\n\ttraverse");
	printf("\n\tshowzone");
	printf("\n\tshowfile");
	printf("\n\tquit");
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Attempts to mount the file entered by the user
int minimount(char* filename) {
	// If fd > 0, mounting was successful
	fd = open(filename, O_RDONLY);
	// If mount was unsuccessful, print error message and return
	if (fd < 0) {
		printf("\n\tCould Not Mount File: %s", filename);
		return 0;
	}
	// If mount was successful, notfiy the user and return
	printf("\n\tMounted File: %s", filename);
	return 1;
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Unmount the currently mounted file
int miniumount() {
	// Notify user that file was unmounted and return
	printf("\n\tUnmounting File");
	close(fd);
	return 0;
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Show the contents of the superblock
int showsuper() {
	// Declare and initialize msb as a superblock structure and allocate enough memory
	struct minix_super_block *msb = (struct minix_super_block*)malloc(sizeof(struct minix_super_block));
	//1024 is the size of each block so offsetting to 1024 skips the boot block
	lseek(fd, 1024, SEEK_SET); 
	// read returns an int so n is needed to catch it
	// also reads all data from the 2nd block (the superblock) into msb
	int n = read(fd, msb, sizeof(struct minix_super_block));
	// Go through the print statements for each attribute of the superblock
	printf("\n\tNumber of inodes:\t%d", msb->s_ninodes);
	printf("\n\tNumber of data zones:\t%d", msb->s_nzones);
	printf("\n\tNumber of imap_blocks:\t%d", msb->s_imap_blocks);
	printf("\n\tNumber of zmap_blocks:\t%d", msb->s_zmap_blocks);
	printf("\n\tFirst data zone:\t%d", msb->s_firstdatazone);
	printf("\n\tLog zone size:\t\t%d", msb->s_log_zone_size);
	printf("\n\tMax size:\t\t%d", msb->s_max_size);
	printf("\n\tMagic:\t\t\t%d", msb->s_magic);
	printf("\n\tState:\t\t\t%d", msb->s_state);
	printf("\n\tZones:\t\t\t%d", msb->s_zones);
	return 1;
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// List each item in the root directory of the file
int simpleTraverse() {
	// Declare and initialize msb as a superblock structure and allocate enough memory
	// This will be needed for the offset
	struct minix_super_block *msb = (struct minix_super_block*)malloc(sizeof(struct minix_super_block));
	// Move the offset past the boot block to the superblock
	lseek(fd, 1024, SEEK_SET);
	// Read the contents of the superblock
	int n = read(fd, msb, sizeof(struct minix_super_block));
	
	// Set the offset to the beginning of the inode section
	// 1024 = block size // 2 = boot block + superblock // imap and zmap are between the superblock and inodes
	int offset = (1024 * (2 + msb->s_imap_blocks + msb->s_zmap_blocks));
	// Declare and initialize mi as an inode structure and allocate memory for it
	struct minix_inode *mi = (struct minix_inode*)malloc(sizeof(struct minix_inode));
	// Move the offset to the pre-calculated offset
	lseek(fd, offset, SEEK_SET);
	// Read the content here to the inode
	n = read(fd, mi, sizeof(struct minix_inode));

	// Declare mde as a directory entry structure
	struct minix_dir_entry *mde;
	// Create a string 1 block long 
	unsigned char inodeBlock[1024];	
	
	// Iterate through each of the 7 zones
	for (int i = 0; i < 7; i++) {
		// Check if current zone is null and if so, terminate the loop
		if (!mi->i_zone[i]) {
			break;
		}
		
		// Move the offset to the start of the data zone
		lseek(fd, mi->i_zone[i] * 1024, SEEK_SET);
		// Read data from the zone into inodeBlock
		read(fd, inodeBlock, sizeof(inodeBlock));

		// Initialize mde as a directory entry structure and offset it by 32 bytes AKA skip . and ..
		mde = (struct minix_dir_entry *)inodeBlock + 32;
		// Runs once for each possible directory entry
		for (int j = 0; j < (1024 / sizeof(*mde)); j++) {
			// If the name is empty (No file), terminate the loop and move on to the next zone
			if(!mde->name[0]) {	
				break;
			}
			// Print the next filename
			printf("\n\t%s", mde->name);
			// Move the pointer to the next file
			mde = mde + sizeof(*mde) + 14;
		}
	}
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
//ADVANCED TRAVERSE FUNCTION
int advancedTraverse() {
	// Declare and initialize msb as a superblock structure and allocate enough memory
	// This will be needed for the offset
	struct minix_super_block *msb = (struct minix_super_block*)malloc(sizeof(struct minix_super_block));
	// Move the offset past the boot block to the superblock
	lseek(fd, 1024, SEEK_SET); 
	// Read the contents of the superblock
	int n = read(fd, msb, sizeof(struct minix_super_block));
	
	
	// Set the offset to the beginning of the inode section
	// 1024 = block size // 2 = boot block + superblock // imap and zmap are between the superblock and inodes
	int offset = (1024 * (2 + msb->s_imap_blocks + msb->s_zmap_blocks));
	// Declare and initialize mi as an inode structure and allocate memory for it
	struct minix_inode *mi = (struct minix_inode*)malloc(sizeof(struct minix_inode));
	// Move the offset to the pre-calculated offset
	lseek(fd, offset, SEEK_SET);
	// Read the content here to the inode
	n = read(fd, mi, sizeof(struct minix_inode));

	// Declare mde as a directory entry structure
	struct minix_dir_entry *mde;
	// Create a string 1 block long 
	unsigned char inodeBlock[1024];	
	
	// Iterate through each of the 7 zones
	for (int i = 0; i < 7; i++) {
		// Check if current zone is null and if so, terminate the loop
		if (!mi->i_zone[i]) {
			break;
		}
		
		// Move the offset to the start of the data zone
		lseek(fd, mi->i_zone[i] * 1024, SEEK_SET);
		// Read data from the zone into inodeBlock
		read(fd, inodeBlock, sizeof(inodeBlock));
		
		// Initialize mde as a directory entry structure and offset it by 32 bytes AKA skip . and ..
		mde = (struct minix_dir_entry *)inodeBlock + 32;
		// Runs once for each possible directory entry
		for (int j = 0; j < (1024 / sizeof(*mde)); j++) {
			// If the name is empty (No file), terminate the loop and move on to the next zone
			if(!mde->name[0]) {	
				break;
			}
			
			// Change the offset to point at each individual inode in the first zone
			offset = (1024 * (2 + msb->s_imap_blocks + msb->s_zmap_blocks)) + (32 * (mde->inode - 1));
			// Move the offset to the beginning of the inode
			lseek(fd, offset, SEEK_SET);
			// Read the currently selected inode into mi
			n = read(fd, mi, sizeof(struct minix_inode));
			
			// Convert the last modified time into epoch time (seconds since January 1, 1970
			time_t epochTime = mi->i_time;
			// Create a structure for the strftime method
			struct tm ts;
			// Create a string to hold the new formatted date
			char lastModified[20];
			// Change the format from raw numbers to month-day-year
			ts = *localtime(&epochTime);
			strftime(lastModified, sizeof(lastModified), "%m-%d-%Y", &ts);
			
			// Create a string to store the permission bits
			char permissionBits[10] = "---------";
			
			// Go through each if statement and test the i_mode to see what permission the file has
			if ((mi->i_mode&S_IFSOCK) == S_IFSOCK) {
				permissionBits[0] = 's';
			}
			if ((mi->i_mode&S_IFLNK) == S_IFLNK) {
				permissionBits[0] = 'l';
			}
			if ((mi->i_mode&S_IFREG) == S_IFREG) {
				permissionBits[0] = '-';
			}
			if ((mi->i_mode&S_IFBLK) == S_IFBLK) {
				permissionBits[0] = 'b';
			}
			if ((mi->i_mode&S_IFDIR) == S_IFDIR) {
				permissionBits[0] = 'd';
			}
			if ((mi->i_mode&S_IFCHR) == S_IFCHR) {
				permissionBits[0] = 'c';
			}
			if ((mi->i_mode&S_IFIFO) == S_IFIFO) {
				permissionBits[0] = 'f';
			}
			
			if ((mi->i_mode&S_IRUSR) == S_IRUSR) {
				permissionBits[1] = 'r';
			}
			if ((mi->i_mode&S_IWUSR) == S_IWUSR) {
				permissionBits[2] = 'w';
			}
			if ((mi->i_mode&S_IXUSR) == S_IXUSR) {
				permissionBits[3] = 'x';
			}
			if ((mi->i_mode&S_IRGRP) == S_IRGRP) {
				permissionBits[4] = 'r';
			}
			if ((mi->i_mode&S_IWGRP) == S_IWGRP) {
				permissionBits[5] = 'w';
			}
			if ((mi->i_mode&S_IXGRP) == S_IXGRP) {
				permissionBits[6] = 'x';
			}
			if ((mi->i_mode&S_IROTH) == S_IROTH) {
				permissionBits[7] = 'r';
			}
			if ((mi->i_mode&S_IWOTH) == S_IWOTH) {
				permissionBits[8] = 'w';
			}
			if ((mi->i_mode&S_IXOTH) == S_IXOTH) {
				permissionBits[9] = 'x';
			}
			
			if (mi->i_mode == 0) {
				break;
			}
			
			// Print all the information regarding the file
			printf("\n\t%s  %hu  %d  %s  %s", permissionBits, mi->i_uid, mi->i_size, lastModified, mde->name);	
			// Move the pointer to the next file
			mde = mde + sizeof(*mde) + 14;	
		}
	}
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
// Show the ASCII content of the user specified zone
int showzone(int userInputZone) {
	// Declare and initialize msb as a superblock structure and allocate enough memory
	// This will be needed for the offset
	struct minix_super_block *msb = (struct minix_super_block*)malloc(sizeof(struct minix_super_block));
	// Move the offset past the boot block to the superblock
	lseek(fd, 1024, SEEK_SET); 
	// Read the contents of the superblock
	int n = read(fd, msb, sizeof(struct minix_super_block));
	
	
	// Set the offset to the beginning of the inode section
	// 1024 = block size // 2 = boot block + superblock // imap and zmap are between the superblock and inodes
	int offset = (1024 * (2 + msb->s_imap_blocks + msb->s_zmap_blocks));
	// Declare and initialize mi as an inode structure and allocate memory for it
	struct minix_inode *mi = (struct minix_inode*)malloc(sizeof(struct minix_inode));
	// Move the offset to the pre-calculated offset
	lseek(fd, offset, SEEK_SET);
	// Read the content here to the inode
	n = read(fd, mi, sizeof(struct minix_inode));
	
	// Create a counter to increment the hex along the left side
	int hexCounter = 0;
	
	// Print the hex characters along the top
	printf("\n\t   0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f\n");
	// Declare mde as a directory entry structure
	struct minix_dir_entry *mde;
	// Create a string 1 block long
	unsigned char inodeBlock[1024];	
	// Iterate through each of the 7 zones
	for (int i = 0; i < 7; i++) {
		// Check if current zone is null and if so, terminate the loop
		if (!mi->i_zone[i]) {
			break;
		}
		// Move offset to start of data zone
		lseek(fd, userInputZone * 1024, SEEK_SET);	
		// Read data from zone into inodeBlock
		read(fd, inodeBlock, sizeof(inodeBlock));	

		// Initialize mde as a directory entry structure
		mde = (struct minix_dir_entry *)inodeBlock;
		//Runs once for each possible entry
		for (int j = 0; j < (1024 / sizeof(*mde)); j++) {		
			
			// Change the offset to point at each individual inode in the first zone
			offset = (1024 * (2 + msb->s_imap_blocks + msb->s_zmap_blocks)) + (32 * (mde->inode - 1));
			// Move the offset to the beginning of the inode
			lseek(fd, offset, SEEK_SET);
			// Read the currently selected inode into mi
			n = read(fd, mi, sizeof(struct minix_inode));
			
			// Terminate the loop once it displays 1024 bytes
			if (hexCounter > 1008) {
				break;
			}
			
			// Print the hex counter along the left side
			printf("\n\t%x\t", hexCounter);
			
			// For each file, iterate through the string name and print each character if ASCII allows it
			for (int k = 0; k < 16; k++) {
				if (isprint(mde->name[k]) != 0) {
					printf("   %c", mde->name[k]);
				} else {
					printf("    ");
				}
			}
			
			// Increase the hex counter by 16
			hexCounter += 16;
			// Print the hex counter along the left side
			printf("\n\t%x", hexCounter);
			// Increase the hex counter by 16
			hexCounter += 16;
			// Move the pointer to the next file
			mde = mde + sizeof(*mde) + 14;		//16 for hex
		}
	}
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
//SHOWFILE FUNCTION
int showfile(char* filename) {
	// Declare and initialize msb as a superblock structure and allocate enough memory
	// This will be needed for the offset
	struct minix_super_block *msb = (struct minix_super_block*)malloc(sizeof(struct minix_super_block));
	// Move the offset past the boot block to the superblock
	lseek(fd, 1024, SEEK_SET); 
	// Read the contents of the superblock
	int n = read(fd, msb, sizeof(struct minix_super_block));
	
	
	// Set the offset to the beginning of the inode section
	// 1024 = block size // 2 = boot block + superblock // imap and zmap are between the superblock and inodes
	int offset = (1024 * (2 + msb->s_imap_blocks + msb->s_zmap_blocks));
	// Declare and initialize mi as an inode structure and allocate memory for it
	struct minix_inode *mi = (struct minix_inode*)malloc(sizeof(struct minix_inode));
	// Move the offset to the pre-calculated offset
	lseek(fd, offset, SEEK_SET);
	// Read the content here to the inode
	n = read(fd, mi, sizeof(struct minix_inode));

	
	// Declare mde as a directory entry structure
	struct minix_dir_entry *mde;
	// Create a string 1 block long
	unsigned char inodeBlock[1024];	
	// Iterate through each of the 7 zones
	for (int i = 0; i < 7; i++) {
		// Check if current zone is null and if so, terminate the loop
		if (!mi->i_zone[i]) {
			break;
		}
		
		// Move offset to start of data zone
		lseek(fd, mi->i_zone[i] * 1024, SEEK_SET);	
		// Read data from zone into inodeBlock
		read(fd, inodeBlock, sizeof(inodeBlock));

		// Initialize mde as a directory entry structure
		mde = (struct minix_dir_entry *)inodeBlock;
		// Runs once for each possible entry
		for (int j = 0; j < (1024 / sizeof(*mde)); j++) {
			// If the directory name is null, terminate the loop
			if(!mde->name[0]) {
				break;
			}
			
			// If the current directory name is equal to user specified file name
			if (strcmp(filename, mde->name) == 0) {
				
				// Create a string 1 block long to hold all the data from the file
				unsigned char fileBlock[1024];
				
				// Move the offset to the start of the inode associated with the user specified file
				lseek(fd, 1024 * (msb->s_firstdatazone + mde->inode - 1), SEEK_SET);
				// Read the data from the file offset into fileBlock
				int nread = read(fd, fileBlock, sizeof(fileBlock));
				printf("\n\t");
				
				
				int col = 0;
				// Run once for every bite read
				for (int m = 0; m < nread; m++) {
					// If there are 16 columns, go to the next line
					if (col == 16) {
						printf("\n\t");
						col = 0;
					}
					// Print the hex value stored in the fileBlock
					printf("%x  ", fileBlock[m]);
					col++;
				}
				return 1;
			}
			// Set the pointer to the next directory entry
			mde = mde + sizeof(*mde) + 14;
		}
	}
	printf("\n\tFile Not Found");
	return 0;
}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
int main() {
	printf("\n-------------------------------------------------------------------");
	// Create a string for the user input
	char userInput[80];
	// Create a string to seperate each piece of the user input
	char* stringToken;
	// Create an array of strings to store the user input
	char* inputArray[2];
	
	// Run the loop until the user terminates it
	while (1) {
		// Set the initial user input to {NULL, NULL}
		for (int i = 0; i < 2; i++) {
			inputArray[i] = NULL;
		}
		// Get the user's input, %[^\n]%*c allows spaces to be accepted by scanf
		printf("\n\nminix:\t");
		scanf("%[^\n]%*c", userInput);
		
		// Begin separating the user input at the spaces
		stringToken = strtok(userInput, " ");
   
		// Insert each piece of the user input into inputArray
		int arrayCount = 0;
		while(stringToken != NULL) {
			inputArray[arrayCount] = stringToken;
			stringToken = strtok(NULL, " ");
			arrayCount++;
		}
		
		// If the user entered "help", run the help function
		if (strcmp(inputArray[0], "help") == 0) {
			help();
			
		// If the user entered "minimount", check for a file and attempt to mount it
		} else if (strcmp(inputArray[0], "minimount") == 0) {
			if (inputArray[1] == NULL) {
				printf("\n\tPlease Enter A Filename");
			} else {
				minimount(inputArray[1]);
			}
		
		// If the user entered "miniumount", check if a file is mounted and if so, unmount it
		} else if (strcmp(inputArray[0], "miniumount") == 0) {
			if (fd <= 0) {
				printf("\n\tNo File Is Currently Mounted");
			} else {
				fd = miniumount();
			}
		
		// If the user entered "showsuper", check if a file is mounted and if so, display the superblock contents
		} else if (strcmp(inputArray[0], "showsuper") == 0) {
			if (fd <= 0) {
				printf("\n\tNo File Mounted");
			} else {
				showsuper();
			}
		
		// If the user entered "traverse", check if a file is mounted and if so, continue
		} else if (strcmp(inputArray[0], "traverse") == 0) {
			if (fd <= 0) {
				printf("\n\tNo File Mounted");
			// If no flag is specified, run the simpleTraverse() function
			} else if (inputArray[1] == NULL) {
				simpleTraverse();
			// If a flag is entered, run the advancedTraverse() function
			} else if (strcmp(inputArray[1], "-l") == 0) {
				advancedTraverse();
			// If the flag isn't valid, notify the user
			} else {
				printf("\n\tThis Is Invalid Input");
			}
		
		// If the user entered "showzone",check if a file is mounted and if so, continue
		} else if (strcmp(inputArray[0], "showzone") == 0) {
			if (fd <= 0) {
				printf("\n\tNo File Mounted");
			// Check if a data zone is specified and if not, notify the user
			} else if (inputArray[1] == NULL) {
				printf("\n\tInvalid Input");
			// If a data zone is entered, run the showzone() function
			} else {
				//atoi() converts a string to an int
				showzone(atoi(inputArray[1]));
			}
		
		// If the user entered "showfile", check if a file is mounted and if so, continue
		} else if (strcmp(inputArray[0], "showfile") == 0) {
			if (fd <= 0) {
				printf("\n\tNo File Mounted");
			// If no filename is entered, notify the user
			} else if (inputArray[1] == NULL) {
				printf("\n\tNo Filename Entered");
			// If a filename is entered, run the showfile() function
			} else {
				showfile(inputArray[1]);
			}
		
		// If a user entered "quit", terminate the program
		} else if (strcmp(inputArray[0], "quit") == 0) {
			printf("\n\n-------------------------------------------------------------------\n");
			return 0;
		
		// If the user's input does not match any commands, notify the  user
		} else {
			printf("\n\tInvalid Input");
		}
	}
}
//-------------------------------------------------------------------------- 