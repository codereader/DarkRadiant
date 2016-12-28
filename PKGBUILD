# Maintainer: codereader <greebo[AT]angua[DOT]at>
pkgname=darkradiant
pkgver=2.1.1
pkgrel=1
epoch=
pkgdesc="Level Editor for Doom 3 (idTech4) and The Dark Mod"
arch=("x86_64")
url="http://darkradiant.sourceforge.net/"
license=('GPL')
depends=("wxgtk>=3.0.0", "ftgl>=2.0.0", "glew>=1.0.0", "boost-libs>=1.46.1", "freealut>=1.0.0", "libvorbis>=1.3.0", "python>=3.5.0", "libsigc++>=2.0.0")
makedepends=("git>=2.0.0", "automake>=1.14", "libtool>=2.4.0", "gcc>=6.0.0", "boost>=1.46.1", "webkitgtk2>=2.4.0")
install=
changelog=
source=("$pkgname-$pkgver::git+https://github.com/codereader/DarkRadiant.git")
md5sums=("SKIP")

build() {
	cd "$pkgname-$pkgver"
	./autogen.sh
	./configure --prefix=/usr --enable-darkmod-plugins
	make --jobs=4
}

check() {
	cd "$pkgname-$pkgver"
	make -k check
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}
