# Maintainer: codereader <greebo[AT]angua[DOT]at>
pkgname=darkradiant
pkgver=2.7.0
pkgrel=1
pkgdesc="Level Editor for Doom 3 (idTech4) and The Dark Mod"
arch=("x86_64")
url="https://www.darkradiant.net/"
license=("GPL")
depends=(wxgtk ftgl glew freealut libvorbis python libsigc++ pybind11)
makedepends=(git boost)
source=("$pkgname::git+https://github.com/codereader/DarkRadiant.git#tag=2.7.0")
md5sums=("SKIP")

build() {
	cd "$pkgname"
	./autogen.sh
	./configure --prefix=/usr --enable-darkmod-plugins
	make
}

check() {
	cd "$pkgname"
	make -k check
}

package() {
	cd "$pkgname"
	make DESTDIR="$pkgdir/" install
}
