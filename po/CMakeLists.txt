find_package(Gettext REQUIRED)

file(GLOB catalogs *.po)

gettext_create_translations("krossword.pot" ALL ${catalogs})
