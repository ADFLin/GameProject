#include "../Engine/Math/BigFloat.h"
#include \u003ciostream\u003e
#include \u003cstring\u003e

int main()
{
\ttypedef TBigFloat\u003c8, 1\u003e BF;
\t
\tstd::cout \u003c\u003c "Testing BigFloat setFromString..." \u003c\u003c std::endl;
\t
\t// Test case from unit test 8.1
\tstd::string input = "1.2345678E-50";
\tBF a = input.c_str();
\tstd::string output;
\ta.getString(output);
\t
\tstd::cout \u003c\u003c "Input:  " \u003c\u003c input \u003c\u003c std::endl;
\tstd::cout \u003c\u003c "Output: " \u003c\u003c output \u003c\u003c std::endl;
\t
\t// Check if output contains the expected digits
\tstd::string digits = output;
\tdigits.erase(std::remove(digits.begin(), digits.end(), '.'), digits.end());
\t
\tif (digits.find("12345678") != std::string::npos)
\t{
\t\tstd::cout \u003c\u003c "✓ Digits found" \u003c\u003c std::endl;
\t}
\telse
\t{
\t\tstd::cout \u003c\u003c "✗ Digits NOT found! Found: " \u003c\u003c digits \u003c\u003c std::endl;
\t}
\t
\tif (output.find("E-50") != std::string::npos)
\t{
\t\tstd::cout \u003c\u003c "✓ Exponent E-50 found" \u003c\u003c std::endl;
\t}
\telse
\t{
\t\tstd::cout \u003c\u003c "✗ Exponent E-50 NOT found!" \u003c\u003c std::endl;
\t}
\t
\t// Round-trip test
\tBF b = output.c_str();
\tBF diff = a;
\tdiff.sub(b);
\t
\tif (diff.isZero())
\t{
\t\tstd::cout \u003c\u003c "✓ Round-trip exact match" \u003c\u003c std::endl;
\t}
\telse
\t{
\t\tBF threshold = "1E-60";
\t\tif (diff \u003c threshold)
\t\t{
\t\t\tstd::cout \u003c\u003c "✓ Round-trip within tolerance" \u003c\u003c std::endl;
\t\t}
\t\telse
\t\t{
\t\t\tstd::cout \u003c\u003c "✗ Round-trip failed!" \u003c\u003c std::endl;
\t\t\tstd::string diffStr;
\t\t\tdiff.getString(diffStr);
\t\t\tstd::cout \u003c\u003c "  Difference: " \u003c\u003c diffStr \u003c\u003c std::endl;
\t\t}
\t}
\t
\treturn 0;
}
