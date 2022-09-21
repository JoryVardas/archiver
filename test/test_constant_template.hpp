#ifndef ARCHIVER_TEST_TESTCONSTANTTEMPLATE_HPP
#define ARCHIVER_TEST_TESTCONSTANTTEMPLATE_HPP
#include <string>

namespace ArchiverTest {
using namespace std::string_literals;
namespace TestData1 {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_1_SHA}${ARCHIVER_TEST_TEST_DATA_1_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_1_SIZE};
}
namespace TestDataSingleExact {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_SHA}${ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_SINGLE_EXACT_SIZE};
}
namespace TestDataSingle {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_SINGLE_SHA}${ARCHIVER_TEST_TEST_DATA_SINGLE_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_SINGLE_SIZE};
}
namespace TestDataNotSingle {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SHA}${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_NOT_SINGLE_SIZE};
}
namespace TestDataCopy {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_COPY_SHA}${ARCHIVER_TEST_TEST_DATA_COPY_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_COPY_SIZE};
}

namespace TestDataAdditional1 {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_ADDITIONAL_1_SHA}${ARCHIVER_TEST_TEST_DATA_ADDITIONAL_1_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_ADDITIONAL_1_SIZE};
}
namespace TestDataAdditionalSingleExact {
const std::string hash =
  "${ARCHIVER_TEST_TEST_DATA_ADDITIONAL_SINGLE_EXACT_SHA}${ARCHIVER_TEST_TEST_DATA_ADDITIONAL_SINGLE_EXACT_BLAKE2B}"s;
const uint64_t size = ${ARCHIVER_TEST_TEST_DATA_ADDITIONAL_SINGLE_EXACT_SIZE};
}

const uint64_t numberAdditionalTestFiles =
  ${ARCHIVER_TEST_NUMBER_ADDITIONAL_FILES};
}

#endif
