# Maintainer: codereader <greebo[AT]angua[DOT]at>
pkgname=darkradiant
pkgver=3.7.0
pkgrel=1
pkgdesc="Level Editor for Doom 3 (idTech4) and The Dark Mod"
arch=("x86_64")
url="https://www.darkradiant.net/"
license=("GPL")
depends=(wxgtk2 ftgl glew freealut libvorbis python libsigc++ eigen)
makedepends=(cmake git)
source=("$pkgname::git+https://github.com/codereader/DarkRadiant.git#tag=3.7.0")
md5sums=("SKIP")

build() {
	cd "$pkgname"
	cmake .
	make
}

package() {
	cd "$pkgname"
	make DESTDIR="$pkgdir/" install
}
