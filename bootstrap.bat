set BOOST=boost_1_67_0

subst w: ..
cd w:\epigram

if not exist %BOOST%.zip powershell -command "start-bitstransfer -source https://dl.bintray.com/boostorg/release/1.65.1/source/%BOOST%.zip -destination %BOOST%.zip"
if not exist external\boost powershell -command "expand-archive %BOOST%.zip -destination external"
cd external
rename %BOOST% boost