#ifndef Z_PDB_ZIP_FILE_H
#define Z_PDB_ZIP_FILE_H

#include <stdio.h>
#include <zip.h>

FILE *open_zip_file(const char *zip_path, const char *entry_path);

int delete_zip_file(const char *zip_path, const char *entry_path);

int save_zip_file(const char *zip_path, const char *entry_path,
                  FILE *source_file);

#endif // Z_PDB_ZIP_FILE_H
