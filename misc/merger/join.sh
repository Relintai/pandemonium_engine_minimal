
mkdir -p out

cd ..
cd ..

python misc/merger/join.py --template misc/merger/pmcore.h.inl --path . --output misc/merger/out/pmcore.h
python misc/merger/join.py --template misc/merger/pmcore.cpp.inl --path . --output misc/merger/out/pmcore.cpp
