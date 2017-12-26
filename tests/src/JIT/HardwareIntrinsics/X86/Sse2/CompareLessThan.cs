// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//

using System;
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
            string methodUnderTestName = nameof(Sse2.CompareLessThan);

            if (Sse2.IsSupported)
            {
                using (var doubleTable = TestTableVector128<double>.Create(testsCount))
                using (var intTable = TestTableVector128<int>.Create(testsCount))
                using (var shortTable = TestTableVector128<short>.Create(testsCount))
                using (var sbyteTable = TestTableVector128<sbyte>.Create(testsCount))
                {
                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<double>, Vector128<double>, Vector128<double>) value = doubleTable[i];
                        var result = Sse2.CompareLessThan(value.Item1, value.Item2);
                        doubleTable.SetOutArray(result);
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<int>, Vector128<int>, Vector128<int>) value = intTable[i];
                        var result = Sse2.CompareLessThan(value.Item1, value.Item2);
                        intTable.SetOutArray(result);
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<short>, Vector128<short>, Vector128<short>) value = shortTable[i];
                        var result = Sse2.CompareLessThan(value.Item1, value.Item2);
                        shortTable.SetOutArray(result);
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<sbyte>, Vector128<sbyte>, Vector128<sbyte>) value = sbyteTable[i];
                        var result = Sse2.CompareLessThan(value.Item1, value.Item2);
                        sbyteTable.SetOutArray(result);
                    }

                    CheckMethod<double> checkDouble = (double x, double y, double z, ref double a) => (a = x < y ? 0xffffffffffffffff : 0) == z;

                    if (!doubleTable.CheckResult(checkDouble))
                    {
                        PrintError(doubleTable, methodUnderTestName, "(double x, double y, double z, ref double a) => (a = x < y ? 0xffffffffffffffff : 0) == z", checkDouble);
                        testResult = Fail;
                    }

                    CheckMethod<int> checkInt32 = (int x, int y, int z, ref int a) =>  (a = x < y ? unchecked((int)0xffffffff) : 0) == z;

                    if (!intTable.CheckResult(checkInt32))
                    {
                        PrintError(intTable, methodUnderTestName, "(int x, int y, int z, ref int a) =>  (a = x < y ? unchecked((int)0xffffffff) : 0) == z);", checkInt32);
                        testResult = Fail;
                    }

                    CheckMethod<short> checkInt16 = (short x, short y, short z, ref short a) => (a = (short)(x < y ? unchecked((short)0xffff) : 0)) == z;

                    if (!shortTable.CheckResult(checkInt16))
                    {
                        PrintError(shortTable, methodUnderTestName, "(short x, short y, short z, ref short a) => (a = (short)(x < y ? unchecked((short)0xffff) : 0)) == z", checkInt16);
                        testResult = Fail;
                    }

                    CheckMethod<sbyte> checkSByte = (sbyte x, sbyte y, sbyte z, ref sbyte a) => (a = x < y ? unchecked((sbyte)0xff) : (sbyte) 0) == z;

                    if (!sbyteTable.CheckResult(checkSByte))
                    {
                        PrintError(sbyteTable, methodUnderTestName, "(sbyte x, sbyte y, sbyte z, ref sbyte a) => (a = x < y ? unchecked((sbyte)0xff) : (sbyte) 0) == z", checkSByte);
                        testResult = Fail;
                    }
                }
            }
            else
            {
                Console.WriteLine($"Sse2.IsSupported: {Sse2.IsSupported}, skipped tests of {typeof(Sse2)}.{methodUnderTestName}");
            }
            return testResult;
        }
    }
}
