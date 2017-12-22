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
    internal static partial class Program
    {
        const int Pass = 100;
        const int Fail = 0;

        static unsafe int Main(string[] args)
        {
            int testResult = Pass;
            int testsCount = 21;
            string methodUnderTestName = nameof(Sse2.AndNot);

            if (Sse2.IsSupported)
            {
                using (var doubleTable = TestTableVector128<double>.Create(testsCount))
                using (var longTable = TestTableVector128<long>.Create(testsCount))
                using (var ulongTable = TestTableVector128<ulong>.Create(testsCount))
                using (var intTable = TestTableVector128<int>.Create(testsCount))
                using (var uintTable = TestTableVector128<uint>.Create(testsCount))
                using (var shortTable = TestTableVector128<short>.Create(testsCount))
                using (var ushortTable = TestTableVector128<ushort>.Create(testsCount))
                using (var sbyteTable = TestTableVector128<sbyte>.Create(testsCount))
                using (var byteTable = TestTableVector128<byte>.Create(testsCount))
                {
                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<double>, Vector128<double>, Vector128<double>) value = doubleTable[i];
                        doubleTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<long>, Vector128<long>, Vector128<long>) value = longTable[i];
                        longTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<ulong>, Vector128<ulong>, Vector128<ulong>) value = ulongTable[i];
                        ulongTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<int>, Vector128<int>, Vector128<int>) value = intTable[i];
                        intTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<uint>, Vector128<uint>, Vector128<uint>) value = uintTable[i];
                        uintTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<short>, Vector128<short>, Vector128<short>) value = shortTable[i];
                        shortTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<ushort>, Vector128<ushort>, Vector128<ushort>) value = ushortTable[i];
                        ushortTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<sbyte>, Vector128<sbyte>, Vector128<sbyte>) value = sbyteTable[i];
                        sbyteTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<byte>, Vector128<byte>, Vector128<byte>) value = byteTable[i];
                        byteTable.SetOutArray(Sse2.AndNot(value.Item1, value.Item2));
                    }

                    CheckMethod<double> checkDouble = (double x, double y, double z, ref double a) => (a = BinaryAndNot(x, y)) == z;

                    if (!doubleTable.CheckResult(checkDouble))
                    {
                        PrintError(doubleTable, methodUnderTestName, "(double x, double y, double z, ref double a) => (a = BinaryAndNot(x, y)) == z", checkDouble);
                        testResult = Fail;
                    }

                    CheckMethod<long> checkLong = (long x, long y, long z, ref long a) => (a = (~x) & y) == z;

                    if (!longTable.CheckResult(checkLong))
                    {
                        PrintError(longTable, methodUnderTestName, "(long x, long y, long z, ref long a) => (a = (~x) & y) == z", checkLong);
                        testResult = Fail;
                    }

                    CheckMethod<ulong> checkUlong = (ulong x, ulong y, ulong z, ref ulong a) => (a = (~x) & y) == z;

                    if (!longTable.CheckResult(checkLong))
                    {
                        PrintError(ulongTable, methodUnderTestName, "(ulong x, ulong y, ulong z, ref ulong a) => (a = (~x) & y) == z", checkUlong);
                        testResult = Fail;
                    }

                    CheckMethod<int> checkInt32 = (int x, int y, int z, ref int a) => (a = (~x) & y) == z;

                    if (!intTable.CheckResult(checkInt32))
                    {
                        PrintError(intTable, methodUnderTestName, "(int x, int y, int z, ref int a) => (a = (~x) & y) == z", checkInt32);
                        testResult = Fail;
                    }

                    CheckMethod<uint> checkUInt32 = (uint x, uint y, uint z, ref uint a) => (a = (~x) & y) == z;

                    if (!uintTable.CheckResult(checkUInt32))
                    {
                        PrintError(uintTable, methodUnderTestName, "(uint x, uint y, uint z, ref uint a) => (a = (~x) & y) == z", checkUInt32);
                        testResult = Fail;
                    }

                    CheckMethod<short> checkInt16 = (short x, short y, short z, ref short a) => (a = (short)((~x) & y)) == z;

                    if (!shortTable.CheckResult(checkInt16))
                    {
                        PrintError(shortTable, methodUnderTestName, "(short x, short y, short z, ref short a) => (a = (short)((~x) & y)) == z", checkInt16);
                        testResult = Fail;
                    }

                    CheckMethod<ushort> checkUInt16 = (ushort x, ushort y, ushort z, ref ushort a) => (a = (ushort)((~x) & y)) == z;

                    if (!ushortTable.CheckResult(checkUInt16))
                    {
                        PrintError(ushortTable, methodUnderTestName, "(ushort x, ushort y, ushort z, ref ushort a) => (a = (ushort)((~x) & y)) == z", checkUInt16);
                        testResult = Fail;
                    }

                    CheckMethod<sbyte> checkSByte = (sbyte x, sbyte y, sbyte z, ref sbyte a) => (a = (sbyte)((~x) & y)) == z;

                    if (!sbyteTable.CheckResult(checkSByte))
                    {
                        PrintError(sbyteTable, methodUnderTestName, "(sbyte x, sbyte y, sbyte z, ref sbyte a) =>(a = (sbyte)((~x) & y)) == z", checkSByte);
                        testResult = Fail;
                    }

                    CheckMethod<byte> checkByte = (byte x, byte y, byte z, ref byte a) => (a = (byte)((~x) & y)) == z;

                    if (!byteTable.CheckResult(checkByte))
                    {
                        PrintError(byteTable, methodUnderTestName, "(byte x, byte y, byte z, ref byte a) =>  (a = (byte)((~x) & y)) == z", checkByte);
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

        public static double BinaryAndNot(double x, double y)
        {
            var xLong = BitConverter.ToInt64(BitConverter.GetBytes(x));
            var yLong = BitConverter.ToInt64(BitConverter.GetBytes(y));
            xLong = ((~xLong) & yLong);
            return BitConverter.ToDouble(BitConverter.GetBytes(xLong));
        }
    }
}
