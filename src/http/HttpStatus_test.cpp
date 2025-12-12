#include "HttpStatus.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(HttpStatusTests, ReasonPhraseCommonStatuses) {
  EXPECT_EQ(http::reasonPhrase(http::S_200_OK), "OK");
  EXPECT_EQ(http::reasonPhrase(http::S_404_NOT_FOUND), "Not Found");
  EXPECT_EQ(http::reasonPhrase(http::S_500_INTERNAL_SERVER_ERROR),
            "Internal Server Error");
  EXPECT_EQ(http::reasonPhrase(http::S_301_MOVED_PERMANENTLY),
            "Moved Permanently");
}

TEST(HttpStatusTests, IntToStatusValidCodes) {
  EXPECT_EQ(http::intToStatus(200), http::S_200_OK);
  EXPECT_EQ(http::intToStatus(404), http::S_404_NOT_FOUND);
  EXPECT_EQ(http::intToStatus(500), http::S_500_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(http::intToStatus(301), http::S_301_MOVED_PERMANENTLY);
}

TEST(HttpStatusTests, IntToStatusInvalidCodeThrows) {
  EXPECT_THROW(http::intToStatus(999), std::invalid_argument);
  EXPECT_THROW(http::intToStatus(100), std::invalid_argument);
  EXPECT_THROW(http::intToStatus(0), std::invalid_argument);
}

TEST(HttpStatusTests, StatusWithReasonFormatsCorrectly) {
  std::string result = http::statusWithReason(http::S_200_OK);
  EXPECT_EQ(result, "200 OK");

  result = http::statusWithReason(http::S_404_NOT_FOUND);
  EXPECT_EQ(result, "404 Not Found");
}

TEST(HttpStatusTests, IsSuccessClassification) {
  EXPECT_TRUE(http::isSuccess(http::S_200_OK));
  EXPECT_TRUE(http::isSuccess(http::S_201_CREATED));
  EXPECT_TRUE(http::isSuccess(http::S_204_NO_CONTENT));
  EXPECT_FALSE(http::isSuccess(http::S_301_MOVED_PERMANENTLY));
  EXPECT_FALSE(http::isSuccess(http::S_404_NOT_FOUND));
  EXPECT_FALSE(http::isSuccess(http::S_500_INTERNAL_SERVER_ERROR));
}

TEST(HttpStatusTests, IsRedirectClassification) {
  EXPECT_TRUE(http::isRedirect(http::S_301_MOVED_PERMANENTLY));
  EXPECT_TRUE(http::isRedirect(http::S_302_FOUND));
  EXPECT_TRUE(http::isRedirect(http::S_307_TEMPORARY_REDIRECT));
  EXPECT_FALSE(http::isRedirect(http::S_200_OK));
  EXPECT_FALSE(http::isRedirect(http::S_404_NOT_FOUND));
}

TEST(HttpStatusTests, IsClientErrorClassification) {
  EXPECT_TRUE(http::isClientError(http::S_400_BAD_REQUEST));
  EXPECT_TRUE(http::isClientError(http::S_404_NOT_FOUND));
  EXPECT_TRUE(http::isClientError(http::S_413_PAYLOAD_TOO_LARGE));
  EXPECT_FALSE(http::isClientError(http::S_200_OK));
  EXPECT_FALSE(http::isClientError(http::S_500_INTERNAL_SERVER_ERROR));
}

TEST(HttpStatusTests, IsServerErrorClassification) {
  EXPECT_TRUE(http::isServerError(http::S_500_INTERNAL_SERVER_ERROR));
  EXPECT_TRUE(http::isServerError(http::S_502_BAD_GATEWAY));
  EXPECT_TRUE(http::isServerError(http::S_503_SERVICE_UNAVAILABLE));
  EXPECT_FALSE(http::isServerError(http::S_200_OK));
  EXPECT_FALSE(http::isServerError(http::S_404_NOT_FOUND));
}

TEST(HttpStatusTests, IsValidStatusCode) {
  EXPECT_TRUE(http::isValidStatusCode(200));
  EXPECT_TRUE(http::isValidStatusCode(404));
  EXPECT_TRUE(http::isValidStatusCode(500));
  EXPECT_FALSE(http::isValidStatusCode(999));
  EXPECT_FALSE(http::isValidStatusCode(100));
  EXPECT_FALSE(http::isValidStatusCode(0));
}
