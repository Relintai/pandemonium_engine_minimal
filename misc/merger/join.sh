
mkdir -p out

cd ..
cd ..

python misc/merger/join.py --template misc/merger/pem_core.h.inl --path . --output misc/merger/out/pem_core.h
python misc/merger/join.py --template misc/merger/pem_core.cpp.inl --path . --output misc/merger/out/pem_core.cpp

#python misc/merger/join.py --template misc/merger/pem_scene.h.inl --path . --output misc/merger/out/pem_scene.h
#python misc/merger/join.py --template misc/merger/pem_scene.cpp.inl --path . --output misc/merger/out/pem_scene.cpp

#python misc/merger/join.py --template misc/merger/pem_scene.h.inl --path . --output misc/merger/out/pem_scene.h
#python misc/merger/join.py --template misc/merger/pem_scene.cpp.inl --path . --output misc/merger/out/pem_scene.cpp