# According to https://docs.u-boot-project.org/en/latest/board/beagle/am67a_beagley_ai.html

# Check your device name with lsblk
SD_CARD_DEVICE="unknown"

umount ${SD_CARD_DEVICE}1
umount ${SD_CARD_DEVICE}2

parted --script ${SD_CARD_DEVICE} \
      mklabel msdos \
      mkpart primary fat16 4MiB 20MiB \
      mkpart primary ext4 20MiB 100% \
      set 1 boot on \
      set 1 bls_boot off \
      set 1 lba on
# Create boot partition
mkfs.vfat ${SD_CARD_DEVICE}1
# Create root partition
mkfs.ext4 ${SD_CARD_DEVICE}2

# Then just copy result folder files to boot partition