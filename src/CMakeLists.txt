add_executable(z_pdb
        zip_file.c
        zip_file.h
        main.c)

find_package(zip REQUIRED)

target_link_libraries(z_pdb PRIVATE zip)
