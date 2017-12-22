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
    public delegate bool CheckMethod<T>(T x, T y, T z, ref T c);
    public unsafe struct TestTableVector128<T> : IDisposable where T : struct
    {
        private const int _stepSize = 16;
        private int _scalarStepSize;

        private GCHandle _inHandle1;
        private GCHandle _inHandle2;
        private GCHandle _outHandle;
        private GCHandle _checkHandle;

        private int _index;

        public T[] inArray1;
        public T[] inArray2;
        public T[] outArray;
        public T[] checkArray;

        public void* InArray1Ptr => _inHandle1.AddrOfPinnedObject().ToPointer();
        public void* InArray2Ptr => _inHandle2.AddrOfPinnedObject().ToPointer();
        public void* OutArrayPtr => _outHandle.AddrOfPinnedObject().ToPointer();
        public void* CheckArrayPtr => _checkHandle.AddrOfPinnedObject().ToPointer();

        public Vector128<T> Vector1 => Unsafe.Read<Vector128<T>>((byte*)InArray1Ptr + (_index * _stepSize));
        public Vector128<T> Vector2 => Unsafe.Read<Vector128<T>>((byte*)InArray2Ptr + (_index * _stepSize));
        public Vector128<T> Vector3 => Unsafe.Read<Vector128<T>>((byte*)OutArrayPtr + (_index * _stepSize));
        public int Index { get => _index; set => _index = value; }

        public void SetOutArray(Vector128<T> value)
        {
            Unsafe.Write((byte*)OutArrayPtr + (_index * _stepSize), value);
        }

        public (Vector128<T>, Vector128<T>, Vector128<T>) this[int index]
        {
            get
            {
                _index = index;
                return new ValueTuple<Vector128<T>, Vector128<T>, Vector128<T>>(Vector1, Vector2, Vector3);
            }
        }

        public ValueTuple<T, T, T, T> GetDataPoint(int index)
        {
            return (inArray1[index], inArray2[index], outArray[index], checkArray[index]);
        }

        public static TestTableVector128<T> Create(int lengthInVectors)
        {
            int length = _stepSize / Marshal.SizeOf<T>() * lengthInVectors;
            var table = new TestTableVector128<T>(new T[length], new T[length], new T[length]);
            table.Initialize();
            return table;
        }

        public TestTableVector128(T[] a, T[] b, T[] c)
        {
            inArray1 = a;
            inArray2 = b;
            outArray = c;
            checkArray = new T[c.Length];
            _scalarStepSize = a.Length / b.Length;
            _index = 0;
            _inHandle1 = GCHandle.Alloc(inArray1, GCHandleType.Pinned);
            _inHandle2 = GCHandle.Alloc(inArray2, GCHandleType.Pinned);
            _outHandle = GCHandle.Alloc(outArray, GCHandleType.Pinned);
            Initialize();
        }

        public void Initialize()
        {
            Random random = new Random(unchecked((int)(DateTime.UtcNow.Ticks & 0x00000000ffffffffl)));
            if (inArray1 is double[])
            {
                var array1 = inArray1 as double[];
                var array2 = inArray2 as double[];
                for (int i = 0; i < inArray1.Length; i++)
                {
                    array1[i] = random.NextDouble() * random.Next();
                    array2[i] = random.NextDouble() * random.Next();
                }
            }
            else if (inArray1 is float[])
            {
                var arrayFloat1 = inArray1 as float[];
                var arrayFloat2 = inArray2 as float[];
                for (int i = 0; i < inArray1.Length; i++)
                {
                    arrayFloat1[i] = (float)(random.NextDouble() * random.Next(ushort.MaxValue));
                    arrayFloat2[i] = (float)(random.NextDouble() * random.Next(ushort.MaxValue));
                }
            }
            else
            {
                random.NextBytes(new Span<byte>(InArray1Ptr, inArray1.Length));
                random.NextBytes(new Span<byte>(InArray2Ptr, inArray1.Length));
            }
        }

        public bool CheckResult(CheckMethod<T> check)
        {
            bool result = true;
            for (int i = 0; i < inArray1.Length; i++)
            {
                if (!check(inArray1[i], inArray2[i], outArray[i], ref checkArray[i]))
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
        }
    }

    internal static partial class Program
    {
        private static void PrintError<T>(TestTableVector128<T> testTable, string functionName = "", string testFuncString = "",
            CheckMethod<T> check = null) where T : struct
        {
            Console.WriteLine($"{typeof(Sse2)}.{functionName} failed on {typeof(T)}:");
            Console.WriteLine($"Test function: {testFuncString}");
            Console.WriteLine($"{ typeof(Sse2)}.{ nameof(Sse2.CompareGreaterThan)} test tuples:");
            for (int i = 0; i < testTable.outArray.Length; i++)
            {
                (T, T, T, T) item = testTable.GetDataPoint(i);
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
