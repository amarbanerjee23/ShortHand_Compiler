#ifndef SHORTHAND_DIAGNOSTIC_CODES_H
#define SHORTHAND_DIAGNOSTIC_CODES_H

namespace shorthand {
namespace diagnostics {

inline constexpr const char *ParserSyntaxError = "SHD2001";
inline constexpr const char *ParserExpectedAIInferBuiltin = "SHD2002";
inline constexpr const char *ParserExpectedGreenAIReportBuiltin = "SHD2003";
inline constexpr const char *ParserSourceLimitExceeded = "SHD2004";
inline constexpr const char *ParserTokenLimitExceeded = "SHD2005";
inline constexpr const char *ParserScannerBudgetExceeded = "SHD2006";
inline constexpr const char *ParserNestingLimitExceeded = "SHD2007";
inline constexpr const char *ScannerUnexpectedCharacter = "SHD2008";
inline constexpr const char *ScannerUnterminatedComment = "SHD2009";
inline constexpr const char *ScannerUnterminatedString = "SHD2010";
inline constexpr const char *ParserDuplicatePackageDeclaration = "SHD2011";
inline constexpr const char *ParserDuplicateModuleDeclaration = "SHD2012";
inline constexpr const char *ParserDuplicateImportAlias = "SHD2013";
inline constexpr const char *ParserDuplicateImportPath = "SHD2014";
inline constexpr const char *ParserModuleDeclarationOrder = "SHD2015";
inline constexpr const char *ParserModuleRequired = "SHD2016";
inline constexpr const char *ModuleManifestNotFound = "SHD2020";
inline constexpr const char *ModuleManifestInvalid = "SHD2021";
inline constexpr const char *ModuleNotFound = "SHD2022";
inline constexpr const char *ModulePathEscape = "SHD2023";
inline constexpr const char *ModuleIdentityMismatch = "SHD2024";
inline constexpr const char *ModuleImportCycle = "SHD2025";
inline constexpr const char *ModuleAmbiguousMapping = "SHD2026";
inline constexpr const char *ModulePackageMismatch = "SHD2027";
inline constexpr const char *ModuleLockfileMismatch = "SHD2028";
inline constexpr const char *ModuleSymbolCollision = "SHD2029";
inline constexpr const char *ModuleExternalRunUnsupported = "SHD2030";
inline constexpr const char *ModuleDependencyIntegrity = "SHD2031";
inline constexpr const char *ModuleLicensePolicy = "SHD2032";
inline constexpr const char *ModuleVersionPolicy = "SHD2033";
inline constexpr const char *ModuleSbomReproducibility = "SHD2034";

inline constexpr const char *SemanticBreakOutsideLoop = "SHD3001";
inline constexpr const char *SemanticContinueOutsideLoop = "SHD3002";
inline constexpr const char *SemanticUnsupportedExecutableType = "SHD3003";
inline constexpr const char *SemanticFunctionArityMismatch = "SHD3004";
inline constexpr const char *SemanticReturnOutsideFunction = "SHD3005";
inline constexpr const char *SemanticGotoUnsupported = "SHD3006";
inline constexpr const char *SemanticReturnTypeMismatch = "SHD3007";
inline constexpr const char *SemanticUndeclaredVariable = "SHD3008";
inline constexpr const char *SemanticDuplicateFunction = "SHD3009";
inline constexpr const char *SemanticInvalidType = "SHD3010";
inline constexpr const char *SemanticStorageOverflow = "SHD3011";
inline constexpr const char *SemanticInvalidConversion = "SHD3012";
inline constexpr const char *SemanticTypeMismatch = "SHD3013";
inline constexpr const char *SemanticInvalidOperator = "SHD3014";
inline constexpr const char *SemanticInvalidCondition = "SHD3015";
inline constexpr const char *SemanticOwnershipViolation = "SHD3016";
inline constexpr const char *SemanticDuplicateLabel = "SHD3017";
inline constexpr const char *SemanticUndefinedLabel = "SHD3018";
inline constexpr const char *SemanticInvalidGotoScope = "SHD3019";
inline constexpr const char *SemanticMissingReturn = "SHD3020";
inline constexpr const char *SemanticDuplicateDeclaration = "SHD3021";
inline constexpr const char *SemanticVoidValueUsed = "SHD3022";
inline constexpr const char *SemanticUndefinedFunction = "SHD3023";
inline constexpr const char *SemanticEnterpriseSyntax = "SHD3024";
inline constexpr const char *SemanticEnterpriseDuplicate = "SHD3025";

inline constexpr const char *AIModelRedeclared = "SHD4001";
inline constexpr const char *AIModelInvalidFormat = "SHD4002";
inline constexpr const char *AIModelInvalidPrecision = "SHD4003";
inline constexpr const char *AIModelInvalidInputShape = "SHD4004";
inline constexpr const char *AIModelInvalidOutputShape = "SHD4005";
inline constexpr const char *AIModelMissingQualityGuardrail = "SHD4006";
inline constexpr const char *AIModelIncompatibleBackend = "SHD4007";
inline constexpr const char *AIModelMissingBackendPreference = "SHD4008";
inline constexpr const char *AIModelNoCompatibleBackend = "SHD4009";
inline constexpr const char *AITensorRedeclared = "SHD4010";
inline constexpr const char *AITensorInvalidShape = "SHD4011";
inline constexpr const char *AIInferUnknownModel = "SHD4012";
inline constexpr const char *AIInferUnknownInputTensor = "SHD4013";
inline constexpr const char *AIInferInputShapeMismatch = "SHD4014";
inline constexpr const char *AIInferOutputShapeMismatch = "SHD4015";
inline constexpr const char *AIInferImplicitOutput = "SHD4016";

inline constexpr const char *GreenAIMissingFunctionalUnit = "SHD5001";
inline constexpr const char *GreenAIMissingSuccessCriteria = "SHD5002";
inline constexpr const char *GreenAIMissingBoundary = "SHD5003";
inline constexpr const char *GreenAIMissingMeasurementOrDataQuality = "SHD5004";
inline constexpr const char *GreenAIInvalidCarbonFactor = "SHD5005";
inline constexpr const char *GreenAIMissingQualityGuardrail = "SHD5006";
inline constexpr const char *GreenAIInvalidClaimsMode = "SHD5007";
inline constexpr const char *GreenAIInvalidBudget = "SHD5008";
inline constexpr const char *GreenAIMeasureUnknownContract = "SHD5009";
inline constexpr const char *GreenAIMeasureExternalBackend = "SHD5010";
inline constexpr const char *C3EcoDuplicateDeclaration = "SHD5101";
inline constexpr const char *C3EcoMissingRequiredField = "SHD5102";
inline constexpr const char *C3EcoInvalidField = "SHD5103";
inline constexpr const char *C3EcoUnsafeCertificationClaim = "SHD5104";

inline constexpr const char *LoweringUndefinedFunction = "SHD6001";

inline constexpr const char *RuntimeArithmeticDomainError = "SHD7001";
inline constexpr const char *RuntimeArrayBounds = "SHD7002";
inline constexpr const char *RuntimeLoopStepZero = "SHD7003";
inline constexpr const char *RuntimeInvalidState = "SHD7004";

}  // namespace diagnostics
}  // namespace shorthand

#endif
