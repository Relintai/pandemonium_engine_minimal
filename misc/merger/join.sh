
mkdir -p out

cd ..
cd ..

python misc/merger/join.py --template misc/merger/pem.h.inl --path . --output misc/merger/out/pem.h
python misc/merger/join.py --template misc/merger/pem.cpp.inl --path . --output misc/merger/out/pem.cpp
