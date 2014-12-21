find_package(Gettext REQUIRED)

file(GLOB catalogs *.po)

#remove the comment to compile translations
#gettext_create_translations("krossword.pot" ALL ${catalogs})
