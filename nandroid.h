#ifndef NANDROID_H
#define NANDROID_H

int nandroid_main(int argc, char** argv);
int nandroid_backup(const char* backup_path, int backup_recovery, int backup_boot, int backup_system, int backup_data, int backup_cache, int backup_sdext, int backup_wimax);
int nandroid_restore(const char* backup_path, int restore_boot, int restore_system, int restore_data, int restore_cache, int restore_sdext, int restore_wimax);
void nandroid_generate_timestamp_path(char* backup_path);

#endif