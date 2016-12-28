# Maintainer: codereader <greebo@angua.at>
pkgname=darkradiant
pkgver=2.1.1
pkgrel=0
epoch=
pkgdesc="Level Editor for Doom 3 (idTech4) and The Dark Mod"
arch=("x86_64")
url="http://darkradiant.sourceforge.net/"
license=('GPL')
groups=()
depends=("wxgtk>=3.0.0", "ftgl>=2.0.0", "glew>=1.0.0", "boost>=1.46.1", "boost-libs>=1.46.1", "freealut>=1.0.0", "webkitgtk2>=2.4.0")
makedepends=("git>=2.0.0", "automake>=1.14", "libtool>=2.4.0", "gcc>=6.0.0")
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=("$pkgname-$pkgver::git+https://github.com/codereader/DarkRadiant.git")
noextract=("tools", "debian")
md5sums=("SKIP")
validpgpkeys=()

prepare() {
	cd "$pkgname-$pkgver"
}

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
