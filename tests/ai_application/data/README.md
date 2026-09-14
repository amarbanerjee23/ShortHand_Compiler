# UCI optical digit recognition data

These are the unmodified `optdigits.tra` and `optdigits.tes` files from
[UCI dataset 80](https://archive.ics.uci.edu/dataset/80/optical+recognition+of+handwritten+digits).
Attribution: E. Alpaydin and C. Kaynak (1998), *Optical Recognition of Handwritten
Digits*, UCI Machine Learning Repository, [DOI 10.24432/C50P49](https://doi.org/10.24432/C50P49).
License: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
Only the local filenames have changed. No personal identifiers are included.

The original author-separated split contains 3,823 training and 1,797 test
images. Each row has 64 integer block counts in [0,16], followed by a digit label
in [0,9]. This is optical digit recognition, not MNIST, ImageNet, detection or
language-model quality evidence. Native evaluation starts with these existing
8x8 block counts; it does not claim to qualify camera capture or NIST bitmap
extraction.

| Local file | Original URL | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| optdigits.tra.csv | https://archive.ics.uci.edu/ml/machine-learning-databases/optdigits/optdigits.tra | 563639 | e1b683cc211604fe8fd8c4417e6a69f31380e0c61d4af22e93cc21e9257ffedd |
| optdigits.tes.csv | https://archive.ics.uci.edu/ml/machine-learning-databases/optdigits/optdigits.tes | 264712 | 6ebb3d2fee246a4e99363262ddf8a00a3c41bee6014c373ed9d9216ba7f651b8 |

The generator verifies immutable size/digest pins before reading either split.
Centroids use training rows only. The test set is never used to choose weights,
normalization, class thresholds or tuning parameters. The predeclared minimum
accuracy is 0.85. Generated model weights and configurations are temporary build
artifacts; native deployment consumes the model directly without Python.
