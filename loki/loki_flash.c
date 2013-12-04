/*
 * loki_flash
 *
 * A sample utility to validate and flash .lok files
 *
 * by Dan Rosenberg (@djrbliss)
 * modified for use in recovery by Seth Shelnutt, adapted by PhilZ
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "loki.h"
#include "../common.h"

int loki_flash(char *partition)
{

	int ifd, aboot_fd, ofd, recovery, offs, match;
	void *orig, *aboot, *patch;
	struct stat st;
	struct boot_img_hdr *hdr;
	struct loki_hdr *loki_hdr;
	char outfile[1024];


	if (!strcmp(partition, BOOT_PARTITION)) {
		recovery = 0;
	} else if (!strcmp(partition, RECOVERY_PARTITION)) {
		recovery = 1;
	} else {
		LOGE("[+] First argument must be \"boot\" or \"recovery\".\n");
		return 1;
	}

	/* Verify input file */
	aboot_fd = open(ABOOT_PARTITION, O_RDONLY);
	if (aboot_fd < 0) {
		LOGE("[-] Failed to open aboot for reading.\n");
		return 1;
	}

	ifd = open(LOKI_IMAGE, O_RDONLY);
	if (ifd < 0) {
		LOGE("[-] Failed to open %s for reading.\n", LOKI_IMAGE);
		return 1;
	}

	/* Map the image to be flashed */
	if (fstat(ifd, &st)) {
		LOGE("[-] fstat() failed.\n");
		return 1;
	}

	orig = mmap(0, (st.st_size + 0x2000 + 0xfff) & ~0xfff, PROT_READ, MAP_PRIVATE, ifd, 0);
	if (orig == MAP_FAILED) {
		LOGE("[-] Failed to mmap Loki image.\n");
		return 1;
	}

	hdr = orig;
	loki_hdr = orig + 0x400;

	/* Verify this is a Loki image */
	if (memcmp(loki_hdr->magic, "LOKI", 4)) {
		LOGE("[-] Input file is not a Loki image.\n");
		return 1;
	}

	/* Verify this is the right type of image */
	if (loki_hdr->recovery != recovery) {
		LOGE("[-] Loki image is not a %s image.\n", recovery ? "recovery" : "boot");
		return 1;
	}

	/* Verify the to-be-patched address matches the known code pattern */
	aboot = mmap(0, 0x40000, PROT_READ, MAP_PRIVATE, aboot_fd, 0);
	if (aboot == MAP_FAILED) {
		LOGE("[-] Failed to mmap aboot.\n");
		return 1;
	}

	match = 0;

	for (offs = 0; offs < 0x10; offs += 0x4) {

		if (hdr->ramdisk_addr < ABOOT_BASE_SAMSUNG)
			patch = hdr->ramdisk_addr - ABOOT_BASE_G2 + aboot + offs;
		else if (hdr->ramdisk_addr < ABOOT_BASE_LG)
			patch = hdr->ramdisk_addr - ABOOT_BASE_SAMSUNG + aboot + offs;
		else
			patch = hdr->ramdisk_addr - ABOOT_BASE_LG + aboot + offs;

		if (patch < aboot || patch > aboot + 0x40000 - 8) {
			LOGE("[-] Invalid .lok file.\n");
			return 1;
		}

		if (!memcmp(patch, PATTERN1, 8) ||
			!memcmp(patch, PATTERN2, 8) ||
			!memcmp(patch, PATTERN3, 8) ||
			!memcmp(patch, PATTERN4, 8) ||
			!memcmp(patch, PATTERN5, 8) ||
			!memcmp(patch, PATTERN6, 8)) {

			match = 1;
			break;
		}
	}

	if (!match) {
		LOGE("[-] Loki aboot version does not match device.\n");
		return 1;
	}

	ui_print("[+] Loki validation passed, flashing image.\n");

	snprintf(outfile, sizeof(outfile),
			 "/dev/block/platform/msm_sdcc.1/by-name/%s",
			 recovery ? "recovery" : "boot");

	ofd = open(outfile, O_WRONLY);
	if (ofd < 0) {
		LOGE("[-] Failed to open output block device.\n");
		return 1;
	}

	if (write(ofd, orig, st.st_size) != st.st_size) {
		LOGE("[-] Failed to write to block device.\n");
		return 1;
	}

	ui_print("[+] Loki flashing complete!\n");

	close(ifd);
	close(aboot_fd);
	close(ofd);

	return 0;
}
