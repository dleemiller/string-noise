from setuptools import setup, Extension, find_packages
from Cython.Build import cythonize

normalize_module = Extension(
    "string_noise.normalize",
    sources=[
        "src/normalize.c",
    ],
    extra_compile_args=["-g"],
)

cython_mask_module = Extension(
    "string_noise.noisers.mask",
    sources=["string_noise/noisers/mask.pyx"],
    extra_compile_args=["-g"],
)

cython_augment_module = Extension(
    "string_noise.noisers.augment",
    sources=["string_noise/noisers/augment.pyx"],
    extra_compile_args=["-g"],
)

cython_random_module = Extension(
    "string_noise.noisers.random",
    sources=["string_noise/noisers/random.pyx"],
    extra_compile_args=["-g"],
)

cython_markov_module = Extension(
    "string_noise.noisers.markov",
    sources=["string_noise/noisers/markov.pyx"],
    extra_compile_args=["-g"],
)

extensions = [
    normalize_module,
    cython_mask_module,
    cython_augment_module,
    cython_random_module,
    cython_markov_module,
]

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
    ext_modules=cythonize(
        extensions,
        compiler_directives={"language_level": 3},
    ),
    include_package_data=True,
    package_data={
        "string_noise.mappings.default": ["*.json", "*.json.gz"],
    },
    zip_safe=False,
)
