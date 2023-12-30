from setuptools import setup, Extension, find_packages

# Define the extension module
string_noise_module = Extension(
    "string_noise.string_noise",
    sources=[
        "src/string_noise.c",
        "src/normalize.c",
        "src/augment.c",
        "src/random.c",
        "src/mask.c",
        "src/tokenizer.c",
        "src/utils.c",
        "src/trie.c",
    ],
)

# Define setup parameters
setup(
    name="string-noise",
    version="0.1",
    author="David Lee Miller",
    author_email="dleemiller@gmail.com",
    description="Python C extension for string text augmentation",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/dleemiller/string-noise",
    license="MIT",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
    ],
    packages=find_packages(),
    ext_modules=[string_noise_module],
)
