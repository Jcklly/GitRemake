	// Struct for the .Manifest
typedef struct _FILES {

	char file_name[257];
	int version_number;
	char file_hash[65];

} files;

	// Struct for .Update
typedef struct _UFILES {
	
	char code[2];
	char file_name[257];
	int version_number;
	char file_hash[65];
} uFiles;
