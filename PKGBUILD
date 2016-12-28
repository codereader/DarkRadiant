# Maintainer: codereader <greebo[AT]angua[DOT]at>
pkgname=darkradiant
pkgver=2.1.0
pkgrel=1
pkgdesc="Level Editor for Doom 3 (idTech4) and The Dark Mod"
arch=("x86_64")
url="http://darkradiant.sourceforge.net/"
license=("GPL")
depends=(wxgtk ftgl glew boost-libs freealut libvorbis python libsigc++)
makedepends=(git boost webkitgtk2)
source=("$pkgname::git+https://github.com/codereader/DarkRadiant.git#branch=2.1.0arch")
md5sums=("SKIP")

build() {
	cd "$pkgname"
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
