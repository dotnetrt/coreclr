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

        internal static unsafe int Main(string[] args)
        {
            int testResult = Pass;
            int testsCount = 21;
            string methodUnderTestName = nameof(Sse2.Multiply);

            if (Sse2.IsSupported)
            {
                using (var doubleTable = TestTableVector128<double>.Create(testsCount))
                using (var uintTable = TestTableVector128<uint>.Create(testsCount))
                {
                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<double>, Vector128<double>, Vector128<double>) value = doubleTable[i];
                        Vector128<double> result = Sse2.Multiply(value.Item1, value.Item2);
                        doubleTable.SetOutArray(result);
                    }

                    for (int i = 0; i < testsCount; i++)
                    {
                        (Vector128<uint>, Vector128<uint>, Vector128<uint>) value = uintTable[i];
                        Vector128<ulong> result = Sse2.Multiply(value.Item1, value.Item2);
                        uintTable.SetOutArray(result);
                    }

                    CheckMethod<uint, long> checkUInt32 = (uint x, uint y, long z, ref long a) => (a = (long)x * y) == z;

                    if (!uintTable.CheckResult(checkUInt32))
                    {
                        PrintError(uintTable, methodUnderTestName, "(short x, short y, short z, ref short a) => (a = (short)(x ^ y)) == z", checkUInt32);
                        testResult = Fail;
                    }

                    CheckMethod<double> checkDouble = (double x, double y, double z, ref double a) => (a = x * y) == z;

                    if (!doubleTable.CheckResult(checkDouble))
                    {
                        PrintError(doubleTable, methodUnderTestName, "(double x, double y, double z, ref double a) => (a = x * y) == z", checkDouble);
                        testResult = Fail;
                    }
                }
                testResult = Fail;
            }
            else
            {
                Console.WriteLine($"Sse2.IsSupported: {Sse2.IsSupported}, skipped tests of {typeof(Sse2)}.{methodUnderTestName}");
            }
            return testResult;
        }
    }
}
