#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <libc/runtime/symbols.internal.h>

#include "zip_file.h"

#define BUFFER_SIZE 4096

void print_error(void) {
  printf("Value of errno: %d\n", errno);
  perror("Message from perror");
}

void print_usage(const char *exe_name) {
  fprintf(stderr, "%s <command>\n", exe_name);
  fprintf(stderr, "\tprint - prints the content of file.txt\n");
  fprintf(stderr, "\twrite - writes stdin to file.txt\n");
  fprintf(stderr, "\tappend - appends stdin to file.txt\n");
}

int main(const int argc, const char **argv) {
  if (argc < 2) {
    const char *exe_name = "z_pdb";
    if (argc == 1) {
      exe_name = argv[0];
    }
    fprintf(stderr, "No arguments\n");
    print_usage(exe_name);
    return 1;
  }

  const char *exe = FindComBinary();
  char exe_path[PATH_MAX];
  strcpy(exe_path, exe);

  {
    char *exe_ext = strrchr(exe_path, '.');
    if (exe_ext != NULL && strcmp(exe_ext, ".tmp") == 0) {
      int tmp_fd = open(exe_path, O_RDONLY);
      if (tmp_fd == -1) {
        printf("Failed to open self.tmp as readonly\n");
        print_error();
        return 1;
      }

      exe_ext[0] = 0;

      int self_fd = open(exe_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
      if (self_fd == -1) {
        close(tmp_fd);
        printf("Failed to create/truncate self\n");
        print_error();
        return 1;
      }

      char buffer[BUFFER_SIZE];
      ssize_t bytes_read;
      while ((bytes_read = read(tmp_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_written = write(self_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
          printf("Failed to write to self\n");
          print_error();
          close(tmp_fd);
          close(self_fd);
          return 1;
        }
      }

      // Check if the read loop terminated because of an error.
      if (bytes_read == -1) {
        printf("Failed to read from self.tmp\n");
        print_error();
        close(tmp_fd);
        close(self_fd);
        return 1;
      }

      printf("Successfully copied self.tmp to self\n");
      close(tmp_fd);
      close(self_fd);

      // char *const tmp_argv[3] = {exe_path, "", NULL};
      // char *const tmp_env[1] = {NULL};
      //
      // execve(exe_path, tmp_argv, tmp_env);

      char command[512];
      // TODO: Ensure this is in fact windows.
      snprintf(command, sizeof(command), "start \"%s\"", exe_path);
      system(command);
    }
  }

  {
    size_t exe_path_end = strlen(exe_path);
    strcpy(&exe_path[exe_path_end], ".tmp");

    if (access(exe_path, F_OK) == 0) {
      if (unlink(exe_path) != 0) {
        printf("Failed to delete tmp file\n");
        print_error();
        return 1;
      }
      return 0;
    }

    exe_path[exe_path_end] = 0;
  }

  {
    int self_fd = open(exe_path, O_RDWR);
    // O_RDONLY
    if (self_fd == -1 && (errno == EPERM || errno == EACCES)) {
      self_fd = open(exe_path, O_RDONLY);
      if (self_fd == -1) {
        printf("Failed to open self as readonly\n");
        print_error();
        return 1;
      }

      size_t exe_path_end = strlen(exe_path);
      strcpy(&exe_path[exe_path_end], ".tmp");
      int tmp_fd = open(exe_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
      if (tmp_fd == -1) {
        close(self_fd);
        printf("Failed to create self.tmp\n");
        print_error();
        return 1;
      }

      char buffer[BUFFER_SIZE];
      ssize_t bytes_read;
      while ((bytes_read = read(self_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_written = write(tmp_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
          printf("Failed to write to self.tmp\n");
          print_error();
          close(tmp_fd);
          close(self_fd);
          return 1;
        }
      }

      // Check if the read loop terminated because of an error.
      if (bytes_read == -1) {
        printf("Failed to read from self\n");
        print_error();
        close(tmp_fd);
        close(self_fd);
        return 1;
      }

      printf("Successfully copied self to self.tmp\n");
      close(tmp_fd);
      close(self_fd);
    }
  }

  FILE *file = open_zip_file(exe_path, "file.txt");
  if (file == NULL) {
    print_error();
    return 1;
  }

  // If print, this is file, else this is stdin.
  FILE *read_file;
  // If print, this is stdout, else this is file.
  FILE *write_file;

  if (strcmp(argv[1], "print") == 0) {
    rewind(file);
    read_file = file;
    write_file = stdout;
  } else if (strcmp(argv[1], "write") == 0) {
    rewind(file);

    read_file = stdin;
    write_file = file;
  } else if (strcmp(argv[1], "append") == 0) {
    fseek(file, 0, SEEK_END);

    read_file = stdin;
    write_file = file;
  } else {
    fclose(file);
    print_usage(exe_path);
    return 1;
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_read;

  // Print
  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, read_file)) > 0) {
    if (fwrite(buffer, 1, bytes_read, write_file) != bytes_read) {
      fprintf(stderr, "Error writing\n");
      print_error();
      fclose(file);
      return 1;
    }
  }

  if (ferror(read_file)) {
    fprintf(stderr, "Error reading\n");
    print_error();
    fclose(file);
    return 1;
  }

  if (fflush(write_file) != 0) {
    fprintf(stderr, "Error flushing output\n");
    print_error();
    fclose(file);
    return 1;
  }

  if (write_file == file) {
    delete_zip_file(exe_path, "file.txt");
    if (save_zip_file(exe_path, "file.txt", file) != 0) {
      print_error();
      printf("EXE: %s\n", exe_path);
      fclose(file);
      return 1;
    }
  }

  fclose(file);

  char *exe_ext = strrchr(exe_path, '.');
  if (exe_ext != NULL && strcmp(exe_ext, ".tmp") == 0) {
    // char *const tmp_argv[3] = {exe_path, "", NULL};
    // char *const tmp_env[1] = {NULL};
    // execve(exe_path, tmp_argv, tmp_env);

    char command[512];
    // TODO: Ensure this is in fact windows.
    snprintf(command, sizeof(command), "start \"%s\"", exe_path);
    system(command);
  }

  return 0;
}
