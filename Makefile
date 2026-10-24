.PHONY: all kernel modules stage1 loader kexec rootfs bundle deploy boot serve

all: kernel modules stage1 loader kexec rootfs bundle

kernel:
	./scripts/apply-kernel-patch.sh
	./scripts/build-kernel.sh

modules:
	./scripts/build-gdma-trigger.sh

stage1:
	./scripts/build-stage1.sh

loader:
	./scripts/build-loader-initramfs.sh

kexec:
	./scripts/build-kexec.sh

rootfs:
	sudo -E ./scripts/build-full-rootfs.sh

bundle:
	./scripts/build-boot-bundle.sh

deploy:
	./scripts/deploy.sh

boot:
	./scripts/boot.sh

serve:
	./scripts/serve-rootfs.sh
