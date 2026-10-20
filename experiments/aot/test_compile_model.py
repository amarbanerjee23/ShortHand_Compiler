#!/usr/bin/env python3
"""Bounded importer, real native execution and buffer contract regressions."""
import concurrent.futures
import ctypes
import os
import pathlib
import subprocess
import tempfile
import unittest

import numpy as np
import onnx
from onnx import helper, numpy_helper
from google.protobuf.message import DecodeError

import compile_model as aot


def fixture(batch=3, inputs=7, outputs=5):
    rng = np.random.default_rng(124)
    w = rng.uniform(-0.5, 0.5, (inputs, outputs)).astype(np.float32)
    w[0,0] = 2.0  # Enables a deterministic finite-input overflow negative.
    b = rng.uniform(-0.5, 0.5, outputs).astype(np.float32)
    graph = helper.make_graph([
        helper.make_node('MatMul', ['X', 'W'], ['Z']),
        helper.make_node('Add', ['Z', 'B'], ['Y'])], 'bounded_affine',
        [helper.make_tensor_value_info('X', onnx.TensorProto.FLOAT, [batch, inputs])],
        [helper.make_tensor_value_info('Y', onnx.TensorProto.FLOAT, [batch, outputs])],
        [numpy_helper.from_array(w, 'W'), numpy_helper.from_array(b, 'B')])
    return helper.make_model(graph, opset_imports=[helper.make_opsetid('', 13)], ir_version=9), w, b


PTR = ctypes.POINTER(ctypes.c_float)


def pointer(a):
    return a.ctypes.data_as(PTR)


class AotTest(unittest.TestCase):
    def test_import_variants_and_exact_constant_roundtrip(self):
        for dims in [(1, 1, 1), (3, 7, 5), (32, 64, 10)]:
            model, w, b = fixture(*dims)
            for reverse in [False, True]:
                if reverse:
                    model.graph.node[1].input.reverse()
                shape, arrays = aot.inspect_model(model.SerializeToString())
                self.assertEqual(shape, dims)
                np.testing.assert_array_equal(arrays[0], w)
                np.testing.assert_array_equal(arrays[1], b)

    def test_reject_unsupported_and_malformed_models(self):
        def invalid(change, reason):
            m, _, _ = fixture()
            change(m)
            with self.subTest(reason=reason), self.assertRaises((ValueError, onnx.checker.ValidationError, DecodeError)):
                aot.inspect_model(m.SerializeToString())
        invalid(lambda m: setattr(m, 'ir_version', 11), 'IR version')
        invalid(lambda m: setattr(m.opset_import[0], 'version', 14), 'opset')
        invalid(lambda m: setattr(m.graph.node[0], 'op_type', 'Conv'), 'operator')
        invalid(lambda m: setattr(m.graph.node[0], 'domain', 'custom'), 'custom domain')
        invalid(lambda m: setattr(m.graph.node[0], 'overload', 'custom'), 'overload')
        invalid(lambda m: m.configuration.add(name='device'), 'device configuration')
        invalid(lambda m: m.graph.node[0].device_configurations.add(configuration_id='device'), 'node device configuration')
        invalid(lambda m: m.graph.node[0].attribute.append(helper.make_attribute('unsafe', 1)), 'attribute')
        invalid(lambda m: setattr(m.graph.input[0].type.tensor_type.shape.dim[0], 'dim_param', 'batch'), 'dynamic')
        invalid(lambda m: setattr(m.graph.input[0].type.tensor_type.shape.dim[0], 'dim_value', 0), 'zero dimension')
        invalid(lambda m: setattr(m.graph.output[0].type.tensor_type.shape.dim[0], 'dim_value', 4), 'batch mismatch')
        invalid(lambda m: setattr(m.graph.input[0].type.tensor_type, 'elem_type', onnx.TensorProto.DOUBLE), 'dtype')
        invalid(lambda m: setattr(m.graph.initializer[0], 'data_location', onnx.TensorProto.EXTERNAL), 'external')
        invalid(lambda m: m.graph.initializer[0].external_data.add(key='location', value='../../secret'), 'external path')
        invalid(lambda m: setattr(m.graph.initializer[0], 'raw_data', b'x'), 'bytes')
        invalid(lambda m: m.graph.initializer[0].float_data.append(1), 'mixed encoding')
        invalid(lambda m: setattr(m.graph.initializer[0], 'raw_data', np.full((7, 5), np.nan, np.float32).tobytes()), 'nonfinite')
        invalid(lambda m: m.graph.node[1].input.__setitem__(0, 'X'), 'wrong wiring')
        invalid(lambda m: setattr(m.graph.initializer[0], 'name', 'B'), 'duplicate constants')
        invalid(lambda m: m.graph.initializer[1].dims.__setitem__(0, 1), 'broadcast subset')
        invalid(lambda m: m.graph.input.append(m.graph.input[0]), 'extra input')
        invalid(lambda m: m.graph.node.append(m.graph.node[1]), 'extra node')
        invalid(lambda m: m.functions.add(name='unused', domain='custom'), 'local functions')
        invalid(lambda m: m.training_info.add(), 'training graph')
        for raw in [b'', b'not protobuf', b'\0' * (aot.MAX_MODEL_BYTES + 1)]:
            with self.assertRaises((ValueError, onnx.checker.ValidationError, DecodeError)):
                aot.inspect_model(raw)
        m, _, _ = fixture()
        with self.assertRaisesRegex(ValueError, 'unknown protobuf fields'):
            aot.inspect_model(m.SerializeToString() + b'\xf8\x07\x01')

    def test_native_results_aliasing_failure_and_threads(self):
        model, weights, bias = fixture()
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            p = root/'input.onnx'
            p.write_bytes(model.SerializeToString())
            generated = root/'artifact'
            manifest = aot.emit(p, generated)
            self.assertEqual(manifest['workspace_bytes'], 3*5*4)
            with self.assertRaisesRegex(ValueError, 'fresh output'):
                aot.emit(p, generated)
            binary = root/'model.so'
            subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-O3', '-fno-fast-math',
                            '-ffp-contract=off', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                            '-fPIC', '-shared', str(generated/'model.cpp'), '-o', str(binary)], check=True)
            run = ctypes.CDLL(str(binary)).sh_aot_infer
            run.argtypes = [PTR, ctypes.c_size_t, PTR, ctypes.c_size_t, PTR, ctypes.c_size_t]
            run.restype = ctypes.c_int
            def case(seed):
                x = np.random.default_rng(seed).uniform(-2, 2, (3, 7)).astype(np.float32)
                y = np.full((3, 5), 91, np.float32)
                work = np.zeros_like(y)
                self.assertEqual(run(pointer(x), x.size, pointer(y), y.size, pointer(work), work.size), 0)
                np.testing.assert_allclose(y, x.astype(np.float64) @ weights.astype(np.float64) + bias,
                                           rtol=1e-4, atol=1e-5)
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(case, range(100)))
            shared = np.arange(40, dtype=np.float32)/40
            original = shared[:21].reshape(3, 7).copy()
            work = np.zeros(15, np.float32)
            self.assertEqual(run(pointer(shared), 21, pointer(shared[2:]), 15, pointer(work), 15), 0)
            np.testing.assert_allclose(shared[2:17].reshape(3, 5), original @ weights + bias, rtol=1e-4, atol=1e-5)
            x, y = np.zeros(21, np.float32), np.full(15, 91, np.float32)
            for args in [(None,21,pointer(y),15,pointer(work),15),
                         (pointer(x),20,pointer(y),15,pointer(work),15),
                         (pointer(x),21,pointer(y),14,pointer(work),15),
                         (pointer(x),21,pointer(y),15,pointer(work),14),
                         (pointer(x),21,pointer(y),15,pointer(x),15),
                         (pointer(x),21,pointer(y),15,pointer(y),15)]:
                self.assertEqual(run(*args), 1)
                np.testing.assert_array_equal(y, np.full(15, 91, np.float32))
            for value in (np.nan, np.inf, -np.inf, np.finfo(np.float32).max):
                x[0] = value
                self.assertEqual(run(pointer(x),21,pointer(y),15,pointer(work),15), 2)
                np.testing.assert_array_equal(y, np.full(15,91,np.float32))
            if os.environ.get('SHORTHAND_AOT_SANITIZERS') == '1':
                source = root/'sanitized.cpp'
                source.write_text('''#include "model.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <thread>
#include <vector>
int main() {
    auto work=[] {
        std::array<float,21> x{};
        std::array<float,17> y{};
        std::array<float,15> scratch{};
        y.fill(91);
        for(int i=0;i<100;++i) assert(sh_aot_infer(x.data(),21,y.data()+1,15,scratch.data(),15)==0);
        assert(y.front()==91 && y.back()==91);
        y.fill(91);
        assert(sh_aot_infer(nullptr,21,y.data()+1,15,scratch.data(),15)==1);
        assert(sh_aot_infer(x.data(),20,y.data()+1,15,scratch.data(),15)==1);
        assert(sh_aot_infer(x.data(),21,y.data()+1,14,scratch.data(),15)==1);
        assert(sh_aot_infer(x.data(),21,y.data()+1,15,scratch.data(),14)==1);
        assert(sh_aot_infer(x.data(),21,y.data()+1,15,x.data(),15)==1);
        assert(sh_aot_infer(x.data(),21,y.data()+1,15,y.data()+1,15)==1);
        for(float bad : {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::max()}) {
            x[0]=bad;
            assert(sh_aot_infer(x.data(),21,y.data()+1,15,scratch.data(),15)==2);
            assert(std::all_of(y.begin(),y.end(),[](float v){return v==91;}));
        }
        x.fill(0);
        assert(sh_aot_infer(x.data(),21,x.data()+2,15,scratch.data(),15)==0);
    };
    std::vector<std::thread> threads;
    for(int i=0;i<4;++i) threads.emplace_back(work);
    for(auto &t:threads) t.join();
}
''')
                sanitizer = root/'sanitized'
                subprocess.run([os.environ.get('CXX','g++'),'-std=c++17','-O1','-g','-pthread',
                    '-fno-fast-math','-ffp-contract=off','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                    '-I'+str(generated),str(source),str(generated/'model.cpp'),'-o',str(sanitizer)],check=True)
                subprocess.run([str(sanitizer)],check=True)


if __name__ == '__main__':
    unittest.main()
