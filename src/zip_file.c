#include <errno.h>

#include "zip_file.h"

FILE *open_zip_file(const char *zip_path, const char *entry_path) {
  FILE *file = tmpfile();
  if (file == NULL) {
    return NULL;
  }

  struct zip_t *zip = zip_open(zip_path, 0, 'r');
  if (zip == NULL) {
    fclose(file);
    return NULL;
  }

  if (zip_entry_open(zip, entry_path) != 0) {
    zip_close(zip);
    // Return an empty file if the file does not exist in zip.
    return file;
  }

  size_t bufsize = zip_entry_size(zip);
  unsigned char *buf = calloc(sizeof(unsigned char), bufsize);
  if (buf == NULL) {
    zip_close(zip);
    fclose(file);
    return NULL;
  }

  zip_entry_noallocread(zip, (void *)buf, bufsize);
  int write_result = fwrite(buf, sizeof(unsigned char), bufsize, file);

  zip_entry_close(zip);
  zip_close(zip);
  free(buf);

  if (write_result != bufsize) {
    fclose(file);
    return NULL;
  }

  return file;
}

int delete_zip_file(const char *zip_path, const char *entry_path) {
  int zip_errnum = 0;
  struct zip_t *zip = zip_openwitherror(zip_path, 0, 'd', &zip_errnum);
  if (zip_errnum == ZIP_EOPNFILE && (errno == EPERM || errno == EACCES)) {
    printf("Zip error: %d\nError: %d\n", zip_errnum, errno);
    return -1;
    // TODO: MMAP
  } else if (zip == NULL) {
    return -1;
  }

  char *entries[] = {(char *)entry_path};
  zip_entries_delete(zip, entries, 1);

  zip_close(zip);
  return 0;

  //  zip_entries_delete(struct zip_t *zip, char *const entries[], size_t len);
}

int save_zip_file(const char *zip_path, const char *entry_path,
                  FILE *source_file) {
  // Ensure the file pointer is at the beginning of the file
  rewind(source_file);

  // Determine the size of the source file
  if (fseek(source_file, 0, SEEK_END) != 0) {
    perror("Failed to seek to end of source file");
    rewind(source_file); // Rewind before returning
    return -1;
  }

  long file_size = ftell(source_file);
  if (file_size < 0) {
    perror("Failed to determine size of source file");
    rewind(source_file);
    return -1;
  }

  // Rewind again to read the content from the start
  rewind(source_file);

  // Allocate a buffer to hold the file's content
  char *buffer = (char *)malloc(file_size);
  if (!buffer) {
    fprintf(stderr, "Failed to allocate memory for file buffer\n");
    return -1;
  }

  // Read the entire file into the buffer
  if (fread(buffer, 1, file_size, source_file) != (size_t)file_size) {
    fprintf(stderr, "Failed to read source file into buffer\n");
    free(buffer);
    return -1;
  }

  int result = 0;

  // Open the zip archive in append mode. 'a' will create the file if it doesn't
  // exist.
  struct zip_t *zip = zip_open(zip_path, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  if (!zip) {
    fprintf(stderr, "Failed to open zip archive at '%s'\n", zip_path);
    free(buffer);
    return -1;
  }

  // Open a new entry in the zip archive
  if (zip_entry_open(zip, entry_path) != 0) {
    fprintf(stderr, "Failed to open zip entry '%s'\n", entry_path);
    result = -1;
  } else {
    // Write the buffer content to the zip entry
    if (zip_entry_write(zip, buffer, file_size) != 0) {
      fprintf(stderr, "Failed to write to zip entry '%s'\n", entry_path);
      result = -1;
    }
    // Close the entry
    zip_entry_close(zip);
  }

  // Clean up
  zip_close(zip);
  free(buffer);

  return result;
}
