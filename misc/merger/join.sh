
mkdir -p out

cd ..
cd ..

python misc/merger/join.py --template misc/merger/pem_core.h.inl --path . --output misc/merger/out/pem_core.h
python misc/merger/join.py --template misc/merger/pem_core.cpp.inl --path . --output misc/merger/out/pem_core.cpp
