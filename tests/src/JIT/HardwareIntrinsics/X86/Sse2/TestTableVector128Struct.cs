// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.X86;

namespace IntelHardwareIntrinsicTest
{
    public delegate bool CheckMethod<T, U>(T x, U y, T z, ref T c);
    public unsafe struct TestTableVector128Struct<T, U> : IDisposable where T : struct where U : struct
    {
        private const int _stepSize = 16;
        private int _scalarStepSize;

        private GCHandle _inHandle1;
        private GCHandle _inHandle2;
        private GCHandle _outHandle;
        private GCHandle _checkHandle;

        private int _index;

        public T[] inArray1;
        public U[] inArray2;
        public T[] outArray;
        public T[] checkArray;

        public void* InArray1Ptr => _inHandle1.AddrOfPinnedObject().ToPointer();
        public void* InArray2Ptr => _inHandle2.AddrOfPinnedObject().ToPointer();
        public void* OutArrayPtr => _outHandle.AddrOfPinnedObject().ToPointer();
        public void* CheckArrayPtr => _checkHandle.AddrOfPinnedObject().ToPointer();

        public Vector128<T> Vector1 => Unsafe.Read<Vector128<T>>((byte*)InArray1Ptr + (_index * _stepSize));
        public U Vector2 => Unsafe.Read<U>((byte*)InArray2Ptr + (_index / _scalarStepSize));
        public Vector128<T> Vector3 => Unsafe.Read<Vector128<T>>((byte*)OutArrayPtr + (_index * _stepSize));
        public Vector128<T> Vector4 => Unsafe.Read<Vector128<T>>((byte*)CheckArrayPtr + (_index * _stepSize));

        public int Index { get => _index; set => _index = value; }

        public void SetOutArray(Vector128<T> value, U scalarValue)
        {
            Unsafe.Write((byte*) OutArrayPtr + (_index * _stepSize), value);
            inArray2[_index] = scalarValue;
        }

        public Vector128<T> this[int index]
        {
            get
            {
                _index = index;
                return Vector1;
            }
        }

        public ValueTuple<T, U, T, T> GetDataPoint(int index)
        {
            return (inArray1[index], inArray2[index / (_stepSize / _scalarStepSize)], outArray[index], checkArray[index]);
        }

        public static TestTableVector128Struct<T, U> Create(int lengthInVectors)
        {
            int length = _stepSize / Marshal.SizeOf<T>() * lengthInVectors;
            var table = new TestTableVector128Struct<T, U>(new T[length], new U[lengthInVectors]);
            table.Initialize();
            return table;
        }

        public TestTableVector128Struct(T[] a, U[] b)
        {
            inArray1 = a;
            inArray2 = b;
            outArray = new T[a.Length];
            checkArray = new T[a.Length];
            _scalarStepSize = a.Length / b.Length;
            _index = 0;
            _inHandle1 = GCHandle.Alloc(inArray1, GCHandleType.Pinned);
            _inHandle2 = GCHandle.Alloc(inArray2, GCHandleType.Pinned);
            _outHandle = GCHandle.Alloc(outArray, GCHandleType.Pinned);
            _checkHandle = GCHandle.Alloc(checkArray, GCHandleType.Pinned);
            Initialize();
        }

        public void Initialize()
        {
            Random random = new Random(unchecked((int)(DateTime.UtcNow.Ticks & 0x00000000ffffffffl)));
            if (inArray1 is double[] array1)
            {
                for (int i = 0; i < inArray1.Length; i++)
                {
                    array1[i] = random.NextDouble() * random.Next();
                }
            }
            else if (inArray1 is float[] arrayFloat1)
            {
                for (int i = 0; i < inArray1.Length; i++)
                {
                    arrayFloat1[i] = (float)(random.NextDouble() * random.Next(ushort.MaxValue));
                }
            }
            else
            {
                random.NextBytes(new Span<byte>(InArray1Ptr, inArray1.Length));
            }
        }

        public bool CheckResult(CheckMethod<T, U> check)
        {
            bool result = true;
            for (int i = 0; i < inArray1.Length; i++)
            {
                if (!check(inArray1[i], inArray2[i / _scalarStepSize], outArray[i], ref checkArray[i]))
                {
                    result = false;
                }
            }
            return result;
        }

        public void Dispose()
        {
            _inHandle1.Free();
            _inHandle2.Free();
            _outHandle.Free();
            _checkHandle.Free();
        }
    }

    internal static partial class Program
    {
        private static void PrintError<T, U>(TestTableVector128Struct<T, U> testTable, string functionName = "", string testFuncString = "",
            CheckMethod<T, U> check = null) where T : struct where U : struct
        {
            Console.WriteLine($"{typeof(Sse2)}.{functionName} failed on {typeof(T)}:");
            Console.WriteLine($"Test function: {testFuncString}");
            Console.WriteLine($"{ typeof(Sse2)}.{ nameof(Sse2.CompareGreaterThan)} test tuples:");
            for (int i = 0; i < testTable.outArray.Length; i++)
            {
                (T, U, T, T) item = testTable.GetDataPoint(i);
                Console.WriteLine(
                    $"({((IFormattable)item.Item1).ToString(null, NumberFormatInfo.InvariantInfo)}, " +
                    $"{((IFormattable)item.Item2).ToString(null, NumberFormatInfo.InvariantInfo)}, " +
                    $"{((IFormattable)item.Item3).ToString(null, NumberFormatInfo.InvariantInfo)}, " +
                    $"{((IFormattable)item.Item4).ToString(null, NumberFormatInfo.InvariantInfo)})" +
                    (check != null ? $"->{check(item.Item1, item.Item2, item.Item3, ref item.Item4)}, " : ", "));
            }
            Console.WriteLine();
        }
    }
}
