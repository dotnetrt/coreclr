// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics.X86;
using System.Runtime.Intrinsics;

namespace IntelHardwareIntrinsicTest
{
    static class Program
    {
        const int Pass = 100;
        const int Fail = 0;

        static unsafe int Main(string[] args)
        {
            int testResult = Pass;

            if (Sse2.IsSupported)
            {
                using (TestTable<double> doubleTable = new TestTable<double>(
                    new double[2] { 1, -5 }, new double[2] { 22, -1 }, new double[2]))
                using (TestTable<int> intTable = new TestTable<int>(
                    new int[4] { 1, -5, 100, 0 }, new int[4] { 22, -1, -50, 0 }, new int[4]))
                using (TestTable<uint> uintTable = new TestTable<uint>(
                    new uint[4] { 1, 5, 100, 0 }, new uint[4] { 22, 1, 50, 0 }, new uint[4]))
                using (TestTable<short> shortTable = new TestTable<short>(
                    new short[8] { 1, -5, 100, 0, 1, -5, 100, 0 },
                    new short[8] { 22, -1, -50, 0, 22, -1, -50, 0 }, new short[8]))
                using (TestTable<ushort> ushortTable = new TestTable<ushort>(
                    new ushort[8] { 11530, 5, 100, 0, 61, 5, 100, 0 },
                    new ushort[8] { 22, 1, 50, 0, 22, 1, 50, 0 }, new ushort[8]))
                using (TestTable<sbyte> sbyteTable = new TestTable<sbyte>(
                    new sbyte[16] { 1, -5, 100, 0, 1, -105, -100, 0, 1, -5, 100, 0, 1, -5, 100, 0 },
                    new sbyte[16] { 22, -1, -10, 0, 22, -1, -21, 0, 21, -1, 10, 0, 47, -1, 11, 125 }, new sbyte[16]))
                using (TestTable<byte> byteTable = new TestTable<byte>(
                    new byte[16] { 33, 5, 100, 0, 251, 255, 100, 0, 173, 5, 117, 31, 219, 255, 100, 255 },
                    new byte[16] { 22, 1, 50, 0, 22, 1, 50, 0, 22, 4, 50, 1, 22, 1, 50, 255 }, new byte[16]))
                {
                    var vd1 = Unsafe.Read<Vector128<double>>(doubleTable.inArray1Ptr);
                    var vd2 = Unsafe.Read<Vector128<double>>(doubleTable.inArray2Ptr);
                    var vd3 = Sse2.CompareEqual(vd1, vd2);
                    Unsafe.Write(doubleTable.outArrayPtr, vd3);

                    var vi1 = Unsafe.Read<Vector128<int>>(intTable.inArray1Ptr);
                    var vi2 = Unsafe.Read<Vector128<int>>(intTable.inArray2Ptr);
                    var vi3 = Sse2.CompareEqual(vi1, vi2);
                    Unsafe.Write(intTable.outArrayPtr, vi3);

                    var vui1 = Unsafe.Read<Vector128<uint>>(uintTable.inArray1Ptr);
                    var vui2 = Unsafe.Read<Vector128<uint>>(uintTable.inArray2Ptr);
                    var vui3 = Sse2.CompareEqual(vui1, vui2);
                    Unsafe.Write(uintTable.outArrayPtr, vui3);

                    var vs1 = Unsafe.Read<Vector128<short>>(shortTable.inArray1Ptr);
                    var vs2 = Unsafe.Read<Vector128<short>>(shortTable.inArray2Ptr);
                    var vs3 = Sse2.CompareEqual(vs1, vs2);
                    Unsafe.Write(shortTable.outArrayPtr, vs3);

                    var vus1 = Unsafe.Read<Vector128<ushort>>(ushortTable.inArray1Ptr);
                    var vus2 = Unsafe.Read<Vector128<ushort>>(ushortTable.inArray2Ptr);
                    var vus3 = Sse2.CompareEqual(vus1, vus2);
                    Unsafe.Write(ushortTable.outArrayPtr, vus3);

                    var vsb1 = Unsafe.Read<Vector128<sbyte>>(sbyteTable.inArray1Ptr);
                    var vsb2 = Unsafe.Read<Vector128<sbyte>>(sbyteTable.inArray2Ptr);
                    var vsb3 = Sse2.CompareEqual(vsb1, vsb2);
                    Unsafe.Write(sbyteTable.outArrayPtr, vsb3);

                    var vb1 = Unsafe.Read<Vector128<byte>>(byteTable.inArray1Ptr);
                    var vb2 = Unsafe.Read<Vector128<byte>>(byteTable.inArray2Ptr);
                    var vb3 = Sse2.CompareEqual(vb1, vb2);
                    Unsafe.Write(byteTable.outArrayPtr, vb3);

                    if (!intTable.CheckResult((x, y, z) => (x == y ? unchecked((int)0xffffffff) : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on int:");
                        foreach (var item in intTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }

                    if (!uintTable.CheckResult((x, y, z) => (x == y ? 0xffffffff : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on uint:");
                        foreach (var item in uintTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }

                    if (!shortTable.CheckResult((x, y, z) => (x == y ? unchecked((short)0xffff) : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on short:");
                        foreach (var item in shortTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }

                    if (!ushortTable.CheckResult((x, y, z) => (x == y ? 0xffff : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on ushort:");
                        foreach (var item in ushortTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }

                    if (!doubleTable.CheckResult((x, y, z) => (x == y ? 0xffffffffffffffff : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on double:");
                        foreach (var item in doubleTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }

                    if (!sbyteTable.CheckResult((x, y, z) => (x == y ? unchecked((sbyte) 0xff) : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on sbyte:");
                        foreach (var item in sbyteTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }

                    if (!byteTable.CheckResult((x, y, z) => (x == y ? 0xff : 0) == z))
                    {
                        Console.WriteLine("SSE2 CompareEqual failed on byte:");
                        foreach (var item in byteTable.outArray)
                        {
                            Console.Write(item + ", ");
                        }
                        Console.WriteLine();
                        testResult = Fail;
                    }
                }
            }
            else
            {
                testResult = Fail;
            }

            return testResult;
        }

        public unsafe struct TestTable<T> : IDisposable where T : struct
        {
            public T[] inArray1;
            public T[] inArray2;
            public T[] outArray;

            public void* inArray1Ptr => inHandle1.AddrOfPinnedObject().ToPointer();
            public void* inArray2Ptr => inHandle2.AddrOfPinnedObject().ToPointer();
            public void* outArrayPtr => outHandle.AddrOfPinnedObject().ToPointer();

            GCHandle inHandle1;
            GCHandle inHandle2;
            GCHandle outHandle;
            public TestTable(T[] a, T[] b, T[] c)
            {
                this.inArray1 = a;
                this.inArray2 = b;
                this.outArray = c;

                inHandle1 = GCHandle.Alloc(inArray1, GCHandleType.Pinned);
                inHandle2 = GCHandle.Alloc(inArray2, GCHandleType.Pinned);
                outHandle = GCHandle.Alloc(outArray, GCHandleType.Pinned);
            }
            public bool CheckResult(Func<T, T, T, bool> check)
            {
                for (int i = 0; i < inArray1.Length; i++)
                {
                    if (!check(inArray1[i], inArray2[i], outArray[i]))
                    {
                        return false;
                    }
                }
                return true;
            }

            public void Dispose()
            {
                inHandle1.Free();
                inHandle2.Free();
                outHandle.Free();
            }
        }

    }
}
