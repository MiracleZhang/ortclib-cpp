/*

 Copyright (c) 2014, Hookflash Inc. / Hookflash Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#include <ortc/internal/types.h>
#include <ortc/internal/platform.h>
#include <ortc/internal/ortc_RTPTypes.h>

#include <ortc/IRTPTypes.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>

#include <cryptopp/sha.h>

#ifdef _DEBUG
#define ASSERT(x) ZS_THROW_BAD_STATE_IF(!(x))
#else
#define ASSERT(x)
#endif //_DEBUG


namespace ortc { ZS_DECLARE_SUBSYSTEM(ortclib) }

namespace ortc
{
  ZS_DECLARE_TYPEDEF_PTR(openpeer::services::IHelper, UseServicesHelper)

  typedef openpeer::services::Hasher<CryptoPP::SHA1> SHA1Hasher;

  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark (helpers)
    #pragma mark

    const float kExactMatchRankAmount = 1000000.0;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRTPTypes::RTPTypesHelper
    #pragma mark

    //-------------------------------------------------------------------------
    void RTPTypesHelper::splitParamsIntoChannels(
                                                const Parameters &params,
                                                ParametersPtrList &outParamsGroupedIntoChannels
                                                )
    {
      typedef IRTPTypes::EncodingID EncodingID;
      typedef std::map<EncodingID, ParametersPtr> StreamMap;

      StreamMap streamMap;

      for (auto iter = params.mEncodingParameters.begin(); iter != params.mEncodingParameters.end(); ++iter)
      {
        auto &encoding = (*iter);
        if (encoding.mDependencyEncodingIDs.size() > 0) continue; // skip all that are dependent on other layers

        ParametersPtr tmpParam(make_shared<Parameters>());
        tmpParam->mCodecs = params.mCodecs;
        tmpParam->mHeaderExtensions = params.mHeaderExtensions;
        tmpParam->mMuxID = params.mMuxID;
        tmpParam->mRTCP = params.mRTCP;
        tmpParam->mEncodingParameters.push_back(encoding);

        outParamsGroupedIntoChannels.push_back(tmpParam);

        if (encoding.mEncodingID.hasData()) {
          streamMap[encoding.mEncodingID] = tmpParam;
        }
      }

      bool missingEntry = false;
      bool foundEntry = false;

      do {
        missingEntry = false;
        foundEntry = false;

        for (auto iter = params.mEncodingParameters.begin(); iter != params.mEncodingParameters.end(); ++iter) {
          auto &encoding = (*iter);

          if (encoding.mDependencyEncodingIDs.size() < 1) continue;         // skip all that do not have any dependencies

          ORTC_THROW_INVALID_PARAMETERS_IF(encoding.mEncodingID.isEmpty())  // if dependencies exist, this layer must have an encoding id

          auto foundExisting = streamMap.find(encoding.mEncodingID);
          if (foundExisting != streamMap.end()) continue;                   // already processed this entry

          for (auto iterDependency = encoding.mDependencyEncodingIDs.begin(); iterDependency != encoding.mDependencyEncodingIDs.end(); ++iterDependency) {
            auto &dependencyID = (*iterDependency);
            auto foundDependency = streamMap.find(dependencyID);
            if (foundDependency == streamMap.end()) continue;

            ParametersPtr existingParam = (*foundDependency).second;
            existingParam->mEncodingParameters.push_back(encoding);

            streamMap[encoding.mEncodingID] = existingParam;
            foundEntry = true;
            goto next;
          }

          missingEntry = true;
          goto next;

        next: {}
        }

      } while ((missingEntry) &&
               (foundEntry));

      ORTC_THROW_INVALID_PARAMETERS_IF((missingEntry) &&
                                       (!foundEntry))

      if (outParamsGroupedIntoChannels.size() < 1) {
        // ensure at least one channel exists
        outParamsGroupedIntoChannels.push_back(make_shared<Parameters>(params));
      }
    }

    //-------------------------------------------------------------------------
    bool RTPTypesHelper::isGeneralizedSSRCCompatibleChange(
                                                          const Parameters &oldParams,
                                                          const Parameters &newParams
                                                          )
    {
      ZS_DECLARE_TYPEDEF_PTR(IRTPTypes::Parameters, Parameters)

      // these changes must cause an SSRC change to occur
      if (oldParams.mMuxID != newParams.mMuxID) return false;

      return true;
    }

    //-------------------------------------------------------------------------
    bool RTPTypesHelper::isCompatibleCodec(
                                          const CodecParameters &oldCodec,
                                          const CodecParameters &newCodec,
                                          float &ioRank
                                          )
    {
      auto supportedCodec = IRTPTypes::toSupportedCodec(oldCodec.mName);

      bool checkMaxPTime = false;
      bool checkNumChannels = true;

      // do codec specific compatibility test(s)
      switch (supportedCodec) {
        case IRTPTypes::SupportedCodec_Unknown:
        {
          break;
        }
        case IRTPTypes::SupportedCodec_Opus:              break;
        case IRTPTypes::SupportedCodec_Isac:              break;
        case IRTPTypes::SupportedCodec_G722:              break;
        case IRTPTypes::SupportedCodec_ILBC:              break;
        case IRTPTypes::SupportedCodec_PCMU:              break;
        case IRTPTypes::SupportedCodec_PCMA:              break;

          // video codecs
        case IRTPTypes::SupportedCodec_VP8:               break;
        case IRTPTypes::SupportedCodec_VP9:               break;
        case IRTPTypes::SupportedCodec_H264:              break;

          // RTX
        case IRTPTypes::SupportedCodec_RTX:               break;

          // FEC
        case IRTPTypes::SupportedCodec_RED:               break;
        case IRTPTypes::SupportedCodec_ULPFEC:            break;

        case IRTPTypes::SupportedCodec_CN:                break;
          
        case IRTPTypes::SupportedCodec_TelephoneEvent:    break;
      }

      if (checkMaxPTime) {
        if (oldCodec.mMaxPTime != newCodec.mMaxPTime) return false;       // not compatible
      } else {
        ioRank += (oldCodec.mMaxPTime == newCodec.mMaxPTime ? 0.01 : -0.01);
      }
      if (checkNumChannels) {
        if (oldCodec.mNumChannels != newCodec.mNumChannels) return false; // not compatible

        ioRank += (oldCodec.mNumChannels == newCodec.mNumChannels ? 0.01 : -0.01);
      }

      ioRank += 0.1;
      return true;
    }

    //-------------------------------------------------------------------------
    Optional<RTPTypesHelper::PayloadType> RTPTypesHelper::pickCodec(
                                                                  Optional<IMediaStreamTrackTypes::Kinds> kind,
                                                                  const Parameters &params
                                                                  )
    {
      typedef IRTPTypes::CodecKinds CodecKinds;

      if (params.mEncodingParameters.size() > 0) {
        auto &encoding = params.mEncodingParameters.front();
        if (encoding.mCodecPayloadType.hasValue()) {
          return encoding.mCodecPayloadType.value();
        }
      }

      for (auto iter = params.mCodecs.begin(); iter != params.mCodecs.end(); ++iter) {
        // pick the first codec found

        auto &codec = (*iter);
        auto supportedCodec = IRTPTypes::toSupportedCodec(codec.mName);

        CodecKinds codecKind = IRTPTypes::getCodecKind(supportedCodec);

        switch (codecKind) {
          case IRTPTypes::CodecKind_Unknown:  break;

            // audio codecs
          case IRTPTypes::CodecKind_Audio:
          {
            if (kind.hasValue()) {
              if (IMediaStreamTrack::Kind_Audio != kind.value()) {
                // not a match and thus cannot choose
                break;
              }
            }
            return codec.mPayloadType;
          }

            // video codecs
          case IRTPTypes::CodecKind_Video:
          {
            if (kind.hasValue()) {
              if (IMediaStreamTrack::Kind_Video != kind.value()) {
                // not a match and thus cannot choose
                break;
              }
            }
            return codec.mPayloadType;
          }
          case IRTPTypes::CodecKind_AV:
          {
            // always choose this payload
            return codec.mPayloadType;
          }

          case IRTPTypes::CodecKind_RTX:
          case IRTPTypes::CodecKind_FEC:
          case IRTPTypes::CodecKind_AudioSupplemental:
          case IRTPTypes::CodecKind_Data:
          {
            // never select
            break;
          }
        }
      }

      return Optional<PayloadType>(); // nothing picked
    }

    //-------------------------------------------------------------------------
    bool RTPTypesHelper::isRankableMatch(
                                        Optional<IMediaStreamTrackTypes::Kinds> kind,
                                        const Parameters &oldParams,
                                        const Parameters &newParams,
                                        float &outRank
                                        )
    {
      outRank = 0;

      if (oldParams.mEncodingParameters.size() < 1) {
        if (newParams.mEncodingParameters.size() > 0) return false;
      }
      if (newParams.mEncodingParameters.size() < 1) {
        if (oldParams.mEncodingParameters.size() > 0) return false;
      }

      if (oldParams.mEncodingParameters.size() < 1) {
        // all codecs must match compatibly
        for (auto iterOldCodec = oldParams.mCodecs.begin(); iterOldCodec != oldParams.mCodecs.end(); ++iterOldCodec) {
          auto &oldCodec = (*iterOldCodec);

          bool found = false;

          for (auto iterNewCodec = newParams.mCodecs.begin(); iterNewCodec != newParams.mCodecs.end(); ++iterNewCodec) {
            auto &newCodec = (*iterNewCodec);

            if (newCodec.mName != oldCodec.mName) continue;               // cannot be the same codec
            if (newCodec.mClockRate != oldCodec.mClockRate) continue;     // cannot be the same codec
            if (newCodec.mPayloadType != oldCodec.mPayloadType) continue; // cannot be the same payload type

            if (!isCompatibleCodec(newCodec, oldCodec, outRank)) return false;

            if (found) {
              ZS_LOG_TRACE(slog("old supplied codec has two matches (thus old params are not compatible)") + ZS_PARAM("old", oldParams.toDebug()) + ZS_PARAM("new", newParams.toDebug()))
              return false;
            }
            // these changes are "incompatible"
          }

          if (!found) {
            ZS_LOG_TRACE(slog("old supplied codec no longer exists (thus old params are not compatible)") + ZS_PARAM("old", oldParams.toDebug()) + ZS_PARAM("new", newParams.toDebug()))
            return false;
          }
        }

        goto check_other_properties;
      }

      // scope: check codec used in encoding params
      {
        auto oldChosen = pickCodec(kind, oldParams);
        auto newChosen = pickCodec(kind, newParams);

        if (!oldChosen.hasValue()) return false;
        if (!newChosen.hasValue()) return false;

        //  payload type cannot change for picked codec
        if (oldChosen.value() != newChosen.value()) return false;

        const CodecParameters *foundOldCodec = NULL;
        const CodecParameters *foundNewCodec = NULL;

        for (auto iterOldCodec = oldParams.mCodecs.begin(); iterOldCodec != oldParams.mCodecs.end(); ++iterOldCodec) {
          auto &oldCodec = (*iterOldCodec);
          if (oldCodec.mPayloadType != oldChosen.value()) continue;
          foundOldCodec = &oldCodec;
          break;
        }
        for (auto iterNewCodec = newParams.mCodecs.begin(); iterNewCodec != newParams.mCodecs.end(); ++iterNewCodec) {
          auto &newCodec = (*iterNewCodec);
          if (newCodec.mPayloadType != newChosen.value()) continue;
          foundNewCodec = &newCodec;
          break;
        }

        // codec must be found in codec list
        if (NULL == foundOldCodec) return false;
        if (NULL == foundNewCodec) return false;

        // make sure meaning of codec is the same
        if (foundOldCodec->mName != foundNewCodec->mName) return false;
        if (foundOldCodec->mClockRate != foundNewCodec->mClockRate) return false;

        if (!isCompatibleCodec(*foundOldCodec, *foundNewCodec, outRank)) return false;
      }

    check_other_properties:
      {
        if (oldParams.mEncodingParameters.size() > 0) {
          ASSERT(newParams.mEncodingParameters.size() > 0)

          auto &oldEncoding = oldParams.mEncodingParameters.front();
          auto &newEncoding = newParams.mEncodingParameters.front();

          // make sure the rid (if specified) matches
          if (oldEncoding.mEncodingID != newEncoding.mEncodingID) return false; // a non-match on the RID is not the same stream

          if (oldEncoding.mEncodingID.hasData()) {
            // this must be an exact match (thus weight so heavily that it will be chosen)
            outRank += kExactMatchRankAmount;
          }
        }

        outRank += (oldParams.mEncodingParameters.size() == newParams.mEncodingParameters.size() ? 1.0 : -0.2);

        for (auto iterOldEncoding = oldParams.mEncodingParameters.begin(); iterOldEncoding != oldParams.mEncodingParameters.end(); ++iterOldEncoding)
        {
          auto &oldEncoding = (*iterOldEncoding);

          bool foundLayer = false;
          for (auto iterNewEncoding = newParams.mEncodingParameters.begin(); iterNewEncoding != newParams.mEncodingParameters.end(); ++iterNewEncoding)
          {
            auto &newEncoding = (*iterNewEncoding);

            if (oldEncoding.mEncodingID != newEncoding.mEncodingID) continue;

            foundLayer = true;
            outRank += (oldEncoding.hash() == newEncoding.hash() ? 0.3 : -0.1);
            break;
          }
          if (!foundLayer) outRank -= 0.2;
        }

        for (auto iterNewEncoding = newParams.mEncodingParameters.begin(); iterNewEncoding != newParams.mEncodingParameters.end(); ++iterNewEncoding)
        {
          auto &newEncoding = (*iterNewEncoding);

          bool foundLayer = false;

          for (auto iterOldEncoding = oldParams.mEncodingParameters.begin(); iterOldEncoding != oldParams.mEncodingParameters.end(); ++iterOldEncoding)
          {
            auto &oldEncoding = (*iterOldEncoding);
            if (oldEncoding.mEncodingID != newEncoding.mEncodingID) continue;

            foundLayer = true;
            break;
          }
          if (!foundLayer) outRank -= 0.2;
        }
      }

      return true;
    }

    //-------------------------------------------------------------------------
    void RTPTypesHelper::calculateDeltaChangesInChannels(
                                                        Optional<IMediaStreamTrackTypes::Kinds> kind,
                                                        const ParametersPtrList &inExistingParamsGroupedIntoChannels,
                                                        const ParametersPtrList &inNewParamsGroupedIntoChannels,
                                                        ParametersPtrList &outUnchangedChannels,
                                                        ParametersPtrList &outNewChannels,
                                                        ParametersPtrPairList &outUpdatedChannels,
                                                        ParametersPtrList &outRemovedChannels
                                                        )
    {
      typedef String Hash;
      typedef std::pair<Hash, ParametersPtr> HashParameterPair;
      typedef std::list<HashParameterPair> HashParameterPairList;

      ZS_LOG_DEBUG(slog("calculating delate changes in channels") + ZS_PARAM("existing", inExistingParamsGroupedIntoChannels.size()) + ZS_PARAM("new", inNewParamsGroupedIntoChannels.size()))

      // scope: pre-screen special cases
      {
        if (inExistingParamsGroupedIntoChannels.size() < 1) {
          ZS_LOG_TRACE(slog("all channels are new"))
          outNewChannels = inNewParamsGroupedIntoChannels;
          return;
        }

        if (inNewParamsGroupedIntoChannels.size() < 1) {
          ZS_LOG_TRACE(slog("all channels are gone"))
          // special case where all are now "gone"
          outRemovedChannels = inExistingParamsGroupedIntoChannels;
          return;
        }
      }

      ParametersPtrList oldList(inExistingParamsGroupedIntoChannels);
      ParametersPtrList newList(inNewParamsGroupedIntoChannels);

      HashParameterPairList newHashedList;

      Parameters::HashOptions hashOptions;

      hashOptions.mHeaderExtensions = false;
      hashOptions.mRTCP = false;

      // scope: calculate hashes for new list
      {
        for (auto iter_doNotUse = newList.begin(); iter_doNotUse != newList.end(); ) {
          auto current = iter_doNotUse;
          ++iter_doNotUse;

          auto params = (*current);
          auto hash = params->hash(hashOptions);

          newHashedList.push_back(HashParameterPair(hash, params));
        }
      }

      // scope: find exact matches
      {
        for (auto iterOld_doNotUse = oldList.begin(); iterOld_doNotUse != oldList.end(); ) {
          auto currentOld = iterOld_doNotUse;
          ++iterOld_doNotUse;

          auto oldParams = (*currentOld);
          auto oldHash = oldParams->hash(hashOptions);

          auto iterNew_doNotUse = newList.begin();
          auto iterNewHash_doNotUse = newHashedList.begin();

          for (; iterNew_doNotUse != newList.begin();) {
            auto currentNew = iterNew_doNotUse;
            ++iterNew_doNotUse;

            auto currentNewHash = iterNewHash_doNotUse;
            ++iterNewHash_doNotUse;

            auto &newHash = (*currentNewHash).first;

            if (oldHash != newHash) continue;   // not an exact match

            auto newParams = (*currentNew);

            if (oldParams->hash() == newParams->hash()) {
              // an exact match
              ZS_LOG_TRACE(slog("parameters are unchanged") + oldParams->toDebug())
              outUnchangedChannels.push_back(oldParams);
            } else {
              ZS_LOG_TRACE(slog("parameters are almost an exact match (but some options have changed)") + ZS_PARAM("old", oldParams->toDebug()) + ZS_PARAM("new", newParams->toDebug()))
              outUpdatedChannels.push_back(OldNewParametersPair(oldParams, newParams));
            }

            // remove this entry since it's now been processed
            oldList.erase(currentOld);
            newList.erase(currentNew);
            newHashedList.erase(currentNewHash);
          }

        }
      }

      newHashedList.clear();  // do not need this list anymore

      // scope: find exact ssrc matching base layer SSRCs
      {
        for (auto iterOld_doNotUse = oldList.begin(); iterOld_doNotUse != oldList.end(); ) {
          auto currentOld = iterOld_doNotUse;
          ++iterOld_doNotUse;

          auto oldParams = (*currentOld);

          for (auto iterNew_doNotUse = newList.begin(); iterNew_doNotUse != newList.begin();) {
            auto currentNew = iterNew_doNotUse;
            ++iterNew_doNotUse;

            auto newParams = (*currentNew);

            if (oldParams->mEncodingParameters.size() < 1) continue;
            if (newParams->mEncodingParameters.size() < 1) continue;

            auto &firstOld = oldParams->mEncodingParameters.front();
            auto &firstNew = newParams->mEncodingParameters.front();

            if (!firstOld.mSSRC.hasValue()) continue;
            if (!firstNew.mSSRC.hasValue()) continue;

            if (firstOld.mSSRC.value() != firstNew.mSSRC.value()) continue;

            ZS_LOG_TRACE(slog("parameters has an SSRC match (thus must match)") + ZS_PARAM("old", oldParams->toDebug()) + ZS_PARAM("new", newParams->toDebug()))
            outUpdatedChannels.push_back(OldNewParametersPair(oldParams, newParams));
          }
        }
      }

      // scope: old params with non-matching SSRC entries must be removed
      {
        for (auto iterOld_doNotUse = oldList.begin(); iterOld_doNotUse != oldList.end(); ) {
          auto currentOld = iterOld_doNotUse;
          ++iterOld_doNotUse;

          auto oldParams = (*currentOld);

          if (oldParams->mEncodingParameters.size() < 1) continue;

          auto &firstOld = oldParams->mEncodingParameters.front();
          if (!firstOld.mSSRC.hasValue()) continue;

          ZS_LOG_TRACE(slog("old parameters did not have an SSRC match (thus must remove)") + oldParams->toDebug())
          outRemovedChannels.push_back(oldParams);

          oldList.erase(currentOld);
        }
      }

      // scope: new params with non-matching SSRC entries must be added
      {
        for (auto iterNew_doNotUse = newList.begin(); iterNew_doNotUse != newList.begin();) {
          auto currentNew = iterNew_doNotUse;
          ++iterNew_doNotUse;

          auto newParams = (*currentNew);

          if (newParams->mEncodingParameters.size() < 1) continue;

          auto &firstNew = newParams->mEncodingParameters.front();
          if (!firstNew.mSSRC.hasValue()) continue;

          ZS_LOG_TRACE(slog("new parameters did not have an SSRC match (thus must add)") + newParams->toDebug())
          outNewChannels.push_back(newParams);

          newList.erase(currentNew);
        }
      }

      // scope: SSRC is not a factor thus check to see what is left is an "update" compatible change (or not)
      {
        if (newList.size() < 1) goto non_compatible_change;
        if (oldList.size() < 1) goto non_compatible_change;

        if (!isGeneralizedSSRCCompatibleChange(*(oldList.front()), *(newList.front()))) goto non_compatible_change;

        goto do_more_matching;

      non_compatible_change:
        {
          outNewChannels = newList;
          outRemovedChannels = oldList;

          ZS_LOG_TRACE(slog("no more compatible changes found") + ZS_PARAM("remove size", outRemovedChannels.size()) + ZS_PARAM("add size", outNewChannels.size()))
          return;
        }

      do_more_matching: {}
      }
      
      // scope: find closest matches
      {
        for (auto iterOld_doNotUse = oldList.begin(); iterOld_doNotUse != oldList.end(); ) {
          auto currentOld = iterOld_doNotUse;
          ++iterOld_doNotUse;

          auto oldParams = (*currentOld);

          float closestRank = 0.0;
          ParametersPtr closestMatchNew;
          auto closestMatchNewIter = newList.end();

          for (auto iterNew_doNotUse = newList.begin(); iterNew_doNotUse != newList.begin();) {
            auto currentNew = iterNew_doNotUse;
            ++iterNew_doNotUse;

            auto newParams = (*currentNew);

            float rank = 0.0;
            if (!isRankableMatch(kind, *oldParams, *newParams, rank)) continue;

            if (!closestMatchNew) {
              // first time match
              closestRank = rank;
              closestMatchNew = newParams;
              closestMatchNewIter = currentNew;
              continue;
            }

            if (rank < closestRank) continue; // this not not a better match

            closestRank = rank;
            closestMatchNew = newParams;
            closestMatchNewIter = currentNew;
          }

          if (closestMatchNew) {
            ZS_LOG_INSANE(slog("close channel params match was found (thus going to update)") + oldParams->toDebug())
            outUpdatedChannels.push_back(OldNewParametersPair(oldParams, closestMatchNew));

            oldList.erase(currentOld);
            newList.erase(closestMatchNewIter);
          } else {
            ZS_LOG_INSANE(slog("old channel params was not found (thus going to remove)") + oldParams->toDebug())

            oldList.erase(currentOld);
            outRemovedChannels.push_back(oldParams);
          }
        }
      }

      // scope: all unprocessed "new" list channels must be added
      {
        outNewChannels = newList;
      }

      ZS_LOG_TRACE(slog("delta calculated for channel params") +
                   ZS_PARAM("kind", (kind.hasValue() ? IMediaStreamTrackTypes::toString(kind.value()) : "")) +
                   ZS_PARAM("unchanged channels", outUnchangedChannels.size()) +
                   ZS_PARAM("new channels", outNewChannels.size()) +
                   ZS_PARAM("udpated channels", outUpdatedChannels.size()) +
                   ZS_PARAM("removed channels", outRemovedChannels.size()))
    }

    //-------------------------------------------------------------------------
    Log::Params RTPTypesHelper::slog(const char *message)
    {
      return Log::Params(message, "ortc::RTPTypesHelper");
    }


  } // namespace internal

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::Capabilities
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::Capabilities::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::Capabilities");

    ElementPtr codecsEl = Element::create("codecs");
    ElementPtr headersEl = Element::create("headers");
    ElementPtr fecMechanismsEl = Element::create("fec mechanisms");

    for (auto iter = mCodecs.begin(); iter != mCodecs.end(); ++iter)
    {
      auto value = (*iter);
      UseServicesHelper::debugAppend(codecsEl, value.toDebug());
    }
    if (codecsEl->hasChildren()) {
      UseServicesHelper::debugAppend(resultEl, codecsEl);
    }

    for (auto iter = mHeaderExtensions.begin(); iter != mHeaderExtensions.end(); ++iter)
    {
      auto value = (*iter);
      UseServicesHelper::debugAppend(headersEl, value.toDebug());
    }
    if (headersEl->hasChildren()) {
      UseServicesHelper::debugAppend(resultEl, headersEl);
    }

    for (auto iter = mFECMechanisms.begin(); iter != mFECMechanisms.end(); ++iter)
    {
      auto value = (*iter);
      UseServicesHelper::debugAppend(fecMechanismsEl, "fec mechanism", value);
    }
    if (fecMechanismsEl->hasChildren()) {
      UseServicesHelper::debugAppend(resultEl,  fecMechanismsEl);
    }

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::Capabilities::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::Capabilities:");

    hasher.update("codecs:0e69ea312f56834897bc0c29eb74bf991bee8d86:");

    for (auto iter = mCodecs.begin(); iter != mCodecs.end(); ++iter)
    {
      auto value = (*iter);
      hasher.update(":");
      hasher.update(value.hash());
    }

    hasher.update("headers:0e69ea312f56834897bc0c29eb74bf991bee8d86:");

    for (auto iter = mHeaderExtensions.begin(); iter != mHeaderExtensions.end(); ++iter)
    {
      auto value = (*iter);
      hasher.update(":");
      hasher.update(value.hash());
    }

    hasher.update("fec:0e69ea312f56834897bc0c29eb74bf991bee8d86:");

    for (auto iter = mFECMechanisms.begin(); iter != mFECMechanisms.end(); ++iter)
    {
      auto value = (*iter);
      hasher.update(":");
      hasher.update(value);
    }

    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::CodecCapability
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::CodecCapability::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::CodecCapability");

    UseServicesHelper::debugAppend(resultEl, "name", mName);
    UseServicesHelper::debugAppend(resultEl, "kind", mKind);
    UseServicesHelper::debugAppend(resultEl, "clock rate", mClockRate);
    UseServicesHelper::debugAppend(resultEl, "preferred payload type", mPreferredPayloadType);
    UseServicesHelper::debugAppend(resultEl, "max ptime", mMaxPTime);
    UseServicesHelper::debugAppend(resultEl, "number of channels", mNumChannels);

    ElementPtr feedbacksEl = Element::create("feedbacks");

    for (auto iter = mFeedback.begin(); iter != mFeedback.end(); ++iter) {
      auto value = (*iter);
      UseServicesHelper::debugAppend(feedbacksEl, value.toDebug());
    }
    if (feedbacksEl->hasChildren()) {
      UseServicesHelper::debugAppend(resultEl, feedbacksEl);
    }

    UseServicesHelper::debugAppend(resultEl, "parameters", (bool)mParameters);
    UseServicesHelper::debugAppend(resultEl, "options", (bool)mOptions);

    UseServicesHelper::debugAppend(resultEl, "max temporal layers", mMaxTemporalLayers);
    UseServicesHelper::debugAppend(resultEl, "max spatial layers", mMaxSpatialLayers);

    UseServicesHelper::debugAppend(resultEl, "svc multistream support", mSVCMultiStreamSupport);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::CodecCapability::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::CodecCapability:");

    hasher.update(mName);
    hasher.update(":");
    hasher.update(mKind);
    hasher.update(":");
    hasher.update(mClockRate);
    hasher.update(":");
    hasher.update(mPreferredPayloadType);
    hasher.update(":");
    hasher.update(mMaxPTime);
    hasher.update(":");
    hasher.update(mNumChannels);

    hasher.update("feedback:0e69ea312f56834897bc0c29eb74bf991bee8d86");

    for (auto iter = mFeedback.begin(); iter != mFeedback.end(); ++iter)
    {
      auto value = (*iter);
      hasher.update(":");
      hasher.update(value.hash());
    }

    hasher.update(":feedback:0e69ea312f56834897bc0c29eb74bf991bee8d86:");

    hasher.update((bool)mParameters);
    hasher.update(":");
    hasher.update((bool)mOptions);
    hasher.update(":");
    hasher.update(mMaxTemporalLayers);
    hasher.update(":");
    hasher.update(mMaxSpatialLayers);
    hasher.update(":");
    hasher.update(mSVCMultiStreamSupport);

    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::OpusCodecCapability
  #pragma mark

  //---------------------------------------------------------------------------
  IRTPTypes::OpusCodecCapabilityPtr IRTPTypes::OpusCodecCapability::convert(AnyPtr any)
  {
    return ZS_DYNAMIC_PTR_CAST(OpusCodecCapability, any);
  }

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::OpusCodecCapability::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::OpusCodecCapability");

    UseServicesHelper::debugAppend(resultEl, "max playback rate", mMaxPlaybackRate);
    UseServicesHelper::debugAppend(resultEl, "stereo", mStereo);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::OpusCodecCapability::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::OpusCodecCapability:");

    hasher.update(mMaxPlaybackRate);
    hasher.update(":");
    hasher.update(mStereo);

    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::VP8CodecCapability
  #pragma mark

  //---------------------------------------------------------------------------
  IRTPTypes::VP8CodecCapabilityPtr IRTPTypes::VP8CodecCapability::convert(AnyPtr any)
  {
    return ZS_DYNAMIC_PTR_CAST(VP8CodecCapability, any);
  }

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::VP8CodecCapability::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::VP8CodecCapability");

    UseServicesHelper::debugAppend(resultEl, "max ft", mMaxFT);
    UseServicesHelper::debugAppend(resultEl, "max fs", mMaxFS);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::VP8CodecCapability::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::VP8CodecCapability:");

    hasher.update(mMaxFT);
    hasher.update(":");
    hasher.update(mMaxFS);

    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::H264CodecCapability
  #pragma mark

  //---------------------------------------------------------------------------
  IRTPTypes::H264CodecCapabilityPtr IRTPTypes::H264CodecCapability::convert(AnyPtr any)
  {
    return ZS_DYNAMIC_PTR_CAST(H264CodecCapability, any);
  }

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::H264CodecCapability::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::H264CodecCapability");

    UseServicesHelper::debugAppend(resultEl, "profile level id", mProfileLevelID);

    if (mPacketizationModes.size() > 0) {
      ElementPtr packetizationModesEl = Element::create("packetization modes");
      for (auto iter = mPacketizationModes.begin(); iter != mPacketizationModes.end(); ++iter) {
        auto &mode = (*iter);
        UseServicesHelper::debugAppend(packetizationModesEl, "packetization mode", mode);
      }
      UseServicesHelper::debugAppend(resultEl, packetizationModesEl);
    }

    UseServicesHelper::debugAppend(resultEl, "max mbps", mMaxMBPS);
    UseServicesHelper::debugAppend(resultEl, "max smbps", mMaxSMBPS);
    UseServicesHelper::debugAppend(resultEl, "max fs", mMaxFS);
    UseServicesHelper::debugAppend(resultEl, "max cpb", mMaxCPB);
    UseServicesHelper::debugAppend(resultEl, "max dpb", mMaxDPB);
    UseServicesHelper::debugAppend(resultEl, "max br", mMaxBR);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::H264CodecCapability::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::H264CodecCapability:");

    hasher.update(mProfileLevelID);
    hasher.update(":packetizationmodes");

    for (auto iter = mPacketizationModes.begin(); iter != mPacketizationModes.end(); ++iter) {
      auto &mode = (*iter);
      hasher.update(":");
      hasher.update(mode);
    }

    hasher.update(":packetizationmodes:");

    hasher.update(mMaxMBPS);
    hasher.update(":");
    hasher.update(mMaxSMBPS);
    hasher.update(":");
    hasher.update(mMaxFS);
    hasher.update(":");
    hasher.update(mMaxCPB);
    hasher.update(":");
    hasher.update(mMaxDPB);
    hasher.update(":");
    hasher.update(mMaxBR);

    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::HeaderExtensions
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::HeaderExtensions::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::HeaderExtensions");

    UseServicesHelper::debugAppend(resultEl, "kind", mKind);
    UseServicesHelper::debugAppend(resultEl, "uri", mURI);
    UseServicesHelper::debugAppend(resultEl, "preferred id", mPreferredID);
    UseServicesHelper::debugAppend(resultEl, "prefer encrypt", mPreferredEncrypt);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::HeaderExtensions::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::HeaderExtensions:");

    hasher.update(mKind);
    hasher.update(":");
    hasher.update(mURI);
    hasher.update(":");
    hasher.update(mPreferredID);
    hasher.update(":");
    hasher.update(mPreferredEncrypt);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::RtcpFeedback
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::RtcpFeedback::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::RtcpFeedback");

    UseServicesHelper::debugAppend(resultEl, "type", mType);
    UseServicesHelper::debugAppend(resultEl, "parameter", mParameter);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::RtcpFeedback::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::RtcpFeedback:");

    hasher.update(mType);
    hasher.update(":");
    hasher.update(mParameter);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::RTCPParameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::RTCPParameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::RTCPParameters");

    UseServicesHelper::debugAppend(resultEl, "ssrc", mSSRC);
    UseServicesHelper::debugAppend(resultEl, "cname", mCName);
    UseServicesHelper::debugAppend(resultEl, "reduced size", mReducedSize);
    UseServicesHelper::debugAppend(resultEl, "mux", mMux);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::RTCPParameters::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::RTCPParameters:");

    hasher.update(mSSRC);
    hasher.update(":");
    hasher.update(mCName);
    hasher.update(":");
    hasher.update(mReducedSize);
    hasher.update(":");
    hasher.update(mMux);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::Parameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::Parameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::Parameters");

    UseServicesHelper::debugAppend(resultEl, "ssrc", mMuxID);

    ElementPtr codecsEl = Element::create("codecs");
    ElementPtr headersEl = Element::create("headers");
    ElementPtr encodingsEl = Element::create("encodings");

    for (auto iter = mCodecs.begin(); iter != mCodecs.end(); ++iter) {
      auto value = (*iter);
      UseServicesHelper::debugAppend(codecsEl, value.toDebug());
    }
    if (codecsEl) UseServicesHelper::debugAppend(resultEl, codecsEl);

    for (auto iter = mHeaderExtensions.begin(); iter != mHeaderExtensions.end(); ++iter) {
      auto value = (*iter);
      UseServicesHelper::debugAppend(headersEl, value.toDebug());
    }
    if (headersEl) UseServicesHelper::debugAppend(resultEl, headersEl);

    for (auto iter = mEncodingParameters.begin(); iter != mEncodingParameters.end(); ++iter) {
      auto value = (*iter);
      UseServicesHelper::debugAppend(encodingsEl, value.toDebug());
    }
    if (encodingsEl) UseServicesHelper::debugAppend(resultEl, encodingsEl);

    UseServicesHelper::debugAppend(resultEl, "rtcp params", mRTCP.toDebug());

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::Parameters::hash(const HashOptions &options) const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::Parameters:");

    if (options.mMuxID) {
      hasher.update(mMuxID);
    }

    if (options.mCodecs) {
      hasher.update("codecs:0e69ea312f56834897bc0c29eb74bf991bee8d86");

      for (auto iter = mCodecs.begin(); iter != mCodecs.end(); ++iter) {
        auto value = (*iter);
        hasher.update(":");
        hasher.update(value.hash());
      }
    }

    if (options.mHeaderExtensions) {
      hasher.update("headers:0e69ea312f56834897bc0c29eb74bf991bee8d86");

      for (auto iter = mHeaderExtensions.begin(); iter != mHeaderExtensions.end(); ++iter) {
        auto value = (*iter);
        hasher.update(":");
        hasher.update(value.hash());
      }
    }

    if (options.mEncodingParameters) {
      hasher.update("encodings:0e69ea312f56834897bc0c29eb74bf991bee8d86");

      for (auto iter = mEncodingParameters.begin(); iter != mEncodingParameters.end(); ++iter) {
        auto value = (*iter);
        hasher.update(":");
        hasher.update(value.hash());
      }
    }

    if (options.mRTCP) {
      hasher.update("rtcp:72b2b94700e10e41adba3cdf656abed590bb65f4:");
      hasher.update(mRTCP.hash());
    }

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::CodecParameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::CodecParameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::CodecParameters");

    UseServicesHelper::debugAppend(resultEl, "name", mName);
    UseServicesHelper::debugAppend(resultEl, "payload type", mPayloadType);
    UseServicesHelper::debugAppend(resultEl, "clock rate", mClockRate);
    UseServicesHelper::debugAppend(resultEl, "max ptime", mMaxPTime);
    UseServicesHelper::debugAppend(resultEl, "number of channels", mNumChannels);

    ElementPtr feedbacksEl = Element::create("feedbacks");

    for (auto iter = mRTCPFeedback.begin(); iter != mRTCPFeedback.end(); ++iter) {
      auto value = (*iter);
      UseServicesHelper::debugAppend(feedbacksEl, value.toDebug());
    }
    if (feedbacksEl) UseServicesHelper::debugAppend(resultEl, feedbacksEl);

    UseServicesHelper::debugAppend(resultEl, "parameters", (bool)mParameters);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::CodecParameters::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::Parameters:");

    hasher.update(mName);
    hasher.update(":");
    hasher.update(mPayloadType);
    hasher.update(":");
    hasher.update(mClockRate);
    hasher.update(":");
    hasher.update(mMaxPTime);
    hasher.update(":");
    hasher.update(mNumChannels);

    hasher.update("feedback:0e69ea312f56834897bc0c29eb74bf991bee8d86");

    for (auto iter = mRTCPFeedback.begin(); iter != mRTCPFeedback.end(); ++iter) {
      auto value = (*iter);
      hasher.update(":");
      hasher.update(value.hash());
    }

    hasher.update(":");
    hasher.update((bool)mParameters);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::HeaderExtensionParameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::HeaderExtensionParameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::HeaderExtensionParameters");

    UseServicesHelper::debugAppend(resultEl, "uri", mURI);
    UseServicesHelper::debugAppend(resultEl, "id", mID);
    UseServicesHelper::debugAppend(resultEl, "encrypt", mEncrypt);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::HeaderExtensionParameters::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::HeaderExtensionParameters:");

    hasher.update(mURI);
    hasher.update(":");
    hasher.update(mID);
    hasher.update(":");
    hasher.update(mEncrypt);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::FECParameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::FECParameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::FECParameters");

    UseServicesHelper::debugAppend(resultEl, "ssrc", mSSRC);
    UseServicesHelper::debugAppend(resultEl, "mechanism", mMechanism);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::FECParameters::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::FECParameters:");

    hasher.update(mSSRC);
    hasher.update(":");
    hasher.update(mMechanism);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::RTXParameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::RTXParameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::RTXParameters");

    UseServicesHelper::debugAppend(resultEl, "ssrc", mSSRC);
    UseServicesHelper::debugAppend(resultEl, "payload type", mPayloadType);
    UseServicesHelper::debugAppend(resultEl, "rtx time", mRTXTime);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::RTXParameters::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::RTXParameters:");

    hasher.update(mSSRC);
    hasher.update(":");
    hasher.update(mPayloadType);
    hasher.update(":");
    hasher.update(mRTXTime);

    return hasher.final();
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::PriorityTypes
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(PriorityTypes type)
  {
    switch (type) {
      case PriorityType_Unknown:        return "";
      case PriorityType_VeryLow:        return "very-low";
      case PriorityType_Low:            return "low";
      case PriorityType_Medium:         return "medium";
      case PriorityType_High:           return "high";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::PriorityTypes IRTPTypes::toPriorityType(const char *type)
  {
    String typeStr(type);

    for (PriorityTypes index = PriorityType_First; index <= PriorityType_Last; index = static_cast<PriorityTypes>(static_cast<std::underlying_type<PriorityTypes>::type>(index) + 1)) {
      if (typeStr == IRTPTypes::toString(index)) return index;
    }

    return PriorityType_Unknown;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::EncodingParameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr IRTPTypes::EncodingParameters::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::IRTPTypes::EncodingParameters");

    UseServicesHelper::debugAppend(resultEl, "ssrc", mSSRC);
    UseServicesHelper::debugAppend(resultEl, "codec payload type", mCodecPayloadType);
    UseServicesHelper::debugAppend(resultEl, "fec", mFEC.hasValue() ? mFEC.value().toDebug() : ElementPtr());
    UseServicesHelper::debugAppend(resultEl, "rtx", mRTX.hasValue() ? mRTX.value().toDebug() : ElementPtr());
    UseServicesHelper::debugAppend(resultEl, "priority", toString(mPriority));
    UseServicesHelper::debugAppend(resultEl, "max bitrate", mMaxBitrate);
    UseServicesHelper::debugAppend(resultEl, "min quality", mMinQuality);
    UseServicesHelper::debugAppend(resultEl, "framerate bias", mFramerateBias);
    UseServicesHelper::debugAppend(resultEl, "active", mActive);
    UseServicesHelper::debugAppend(resultEl, "encoding id", mEncodingID);

    ElementPtr depedenciesEl = Element::create("dependency encoding ids");

    for (auto iter = mDependencyEncodingIDs.begin(); iter != mDependencyEncodingIDs.end(); ++iter) {
      auto value = (*iter);
      UseServicesHelper::debugAppend(depedenciesEl, "dependency encoding id", value);
    }
    if (depedenciesEl->hasChildren()) UseServicesHelper::debugAppend(resultEl, depedenciesEl);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String IRTPTypes::EncodingParameters::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ortc::IRTPTypes::EncodingParameters:");

    hasher.update(mSSRC);
    hasher.update(":");
    hasher.update(mCodecPayloadType);
    hasher.update(":");
    hasher.update(mFEC.hasValue() ? mFEC.value().hash() : String());
    hasher.update(":");
    hasher.update(mRTX.hasValue() ? mRTX.value().hash() : String());
    hasher.update(":");
    hasher.update(mPriority);
    hasher.update(":");
    hasher.update(mMaxBitrate);
    hasher.update(":");
    hasher.update(mMinQuality);
    hasher.update(":");
    hasher.update(mFramerateBias);
    hasher.update(":");
    hasher.update(mActive);
    hasher.update(":");
    hasher.update(mEncodingID);

    for (auto iter = mDependencyEncodingIDs.begin(); iter != mDependencyEncodingIDs.end(); ++iter) {
      auto value = (*iter);
      hasher.update(":");
      hasher.update(value);
    }

    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::CodecKinds
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(CodecKinds kind)
  {
    switch (kind) {
      case CodecKind_Unknown:           return "";
      case CodecKind_Audio:             return "audio";
      case CodecKind_Video:             return "video";
      case CodecKind_AV:                return "av";
      case CodecKind_RTX:               return "rtx";
      case CodecKind_FEC:               return "fec";
      case CodecKind_AudioSupplemental: return "asupplemental";
      case CodecKind_Data:              return "data";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::CodecKinds IRTPTypes::toCodecKind(const char *kind)
  {
    String kindStr(kind);

    for (CodecKinds index = CodecKind_First; index <= CodecKind_Last; index = static_cast<CodecKinds>(static_cast<std::underlying_type<CodecKinds>::type>(index) + 1)) {
      if (kindStr == IRTPTypes::toString(index)) return index;
    }

    return CodecKind_Unknown;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::SupportedCodecs
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(SupportedCodecs codec)
  {
    switch (codec) {
      case SupportedCodec_Unknown:            return "";

      case SupportedCodec_Opus:               return "opus";
      case SupportedCodec_Isac:               return "isac";
      case SupportedCodec_G722:               return "g722";
      case SupportedCodec_ILBC:               return "ilbc";
      case SupportedCodec_PCMU:               return "pcmu";
      case SupportedCodec_PCMA:               return "pcma";

      case SupportedCodec_VP8:                return "VP8";
      case SupportedCodec_VP9:                return "VP9";
      case SupportedCodec_H264:               return "H264";

      case SupportedCodec_RTX:                return "rtx";

      case SupportedCodec_RED:                return "red";
      case SupportedCodec_ULPFEC:             return "ulpfec";

      case SupportedCodec_CN:                 return "cn";
        
      case SupportedCodec_TelephoneEvent:     return "telephone-event";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::SupportedCodecs IRTPTypes::toSupportedCodec(const char *codec)
  {
    String codecStr(codec);

    for (SupportedCodecs index = SupportedCodec_First; index <= SupportedCodec_Last; index = static_cast<SupportedCodecs>(static_cast<std::underlying_type<SupportedCodecs>::type>(index) + 1)) {
      if (0 == codecStr.compareNoCase(IRTPTypes::toString(index))) return index;
    }

    return SupportedCodec_Unknown;
  }

  //---------------------------------------------------------------------------
  IRTPTypes::CodecKinds IRTPTypes::getCodecKind(SupportedCodecs codec)
  {
    switch (codec) {
      case SupportedCodec_Unknown:            return CodecKind_Unknown;

      case SupportedCodec_Opus:               return CodecKind_Audio;
      case SupportedCodec_Isac:               return CodecKind_Audio;
      case SupportedCodec_G722:               return CodecKind_Audio;
      case SupportedCodec_ILBC:               return CodecKind_Audio;
      case SupportedCodec_PCMU:               return CodecKind_Audio;
      case SupportedCodec_PCMA:               return CodecKind_Audio;

      case SupportedCodec_VP8:                return CodecKind_Video;
      case SupportedCodec_VP9:                return CodecKind_Video;
      case SupportedCodec_H264:               return CodecKind_Video;

      case SupportedCodec_RTX:                return CodecKind_RTX;

      case SupportedCodec_RED:                return CodecKind_FEC;
      case SupportedCodec_ULPFEC:             return CodecKind_FEC;

      case SupportedCodec_CN:                 return CodecKind_AudioSupplemental;

      case SupportedCodec_TelephoneEvent:     return CodecKind_AudioSupplemental;
    }
    
    return CodecKind_Unknown;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::ReservedCodecPayloadTypes
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(ReservedCodecPayloadTypes reservedCodec)
  {
    switch (reservedCodec) {
      case ReservedCodecPayloadType_Unknown:      return "";
      case ReservedCodecPayloadType_PCMU_8000:    return "pcmu";

      case ReservedCodecPayloadType_GSM_8000:     return "gsm";
      case ReservedCodecPayloadType_G723_8000:    return "g723";
      case ReservedCodecPayloadType_DVI4_8000:    return "dvi4";
      case ReservedCodecPayloadType_DVI4_16000:   return "dvi4";
      case ReservedCodecPayloadType_LPC_8000:     return "lpc";
      case ReservedCodecPayloadType_PCMA_8000:    return "pcma";
      case ReservedCodecPayloadType_G722_8000:    return "g722";
      case ReservedCodecPayloadType_L16_44100_2:  return "l16";
      case ReservedCodecPayloadType_L16_44100_1:  return "l16";
      case ReservedCodecPayloadType_QCELP_8000:   return "qcelp";
      case ReservedCodecPayloadType_CN_8000:      return "cn";
      case ReservedCodecPayloadType_MPA_90000:    return "mpa";
      case ReservedCodecPayloadType_G728_8000:    return "g728";
      case ReservedCodecPayloadType_DVI4_11025:   return "dvi4";
      case ReservedCodecPayloadType_DVI4_22050:   return "dvi4";
      case ReservedCodecPayloadType_G729_8000:    return "g729";

      case ReservedCodecPayloadType_CelB_90000:   return "CelB";
      case ReservedCodecPayloadType_JPEG_90000:   return "jpeg";

      case ReservedCodecPayloadType_nv_90000:     return "nv";

      case ReservedCodecPayloadType_H261_90000:   return "H261";
      case ReservedCodecPayloadType_MPV_90000:    return "MPV";
      case ReservedCodecPayloadType_MP2T_90000:   return "MP2T";
      case ReservedCodecPayloadType_H263_90000:   return "H263";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::ReservedCodecPayloadTypes IRTPTypes::toReservedCodec(const char *encodingName)
  {
    static ReservedCodecPayloadTypes types[] = {
      ReservedCodecPayloadType_PCMU_8000,

      ReservedCodecPayloadType_GSM_8000,
      ReservedCodecPayloadType_G723_8000,
      ReservedCodecPayloadType_DVI4_8000,
      ReservedCodecPayloadType_DVI4_16000,
      ReservedCodecPayloadType_LPC_8000,
      ReservedCodecPayloadType_PCMA_8000,
      ReservedCodecPayloadType_G722_8000,
      ReservedCodecPayloadType_L16_44100_2,
      ReservedCodecPayloadType_L16_44100_1,
      ReservedCodecPayloadType_QCELP_8000,
      ReservedCodecPayloadType_CN_8000,
      ReservedCodecPayloadType_MPA_90000,
      ReservedCodecPayloadType_G728_8000,
      ReservedCodecPayloadType_DVI4_11025,
      ReservedCodecPayloadType_DVI4_22050,
      ReservedCodecPayloadType_G729_8000,

      ReservedCodecPayloadType_CelB_90000,
      ReservedCodecPayloadType_JPEG_90000,

      ReservedCodecPayloadType_nv_90000,

      ReservedCodecPayloadType_H261_90000,
      ReservedCodecPayloadType_MPV_90000,
      ReservedCodecPayloadType_MP2T_90000,
      ReservedCodecPayloadType_H263_90000,

      ReservedCodecPayloadType_Unknown
    };

    String encodingNameStr(encodingName);

    for (size_t index = 0; ReservedCodecPayloadType_Unknown != types[index]; ++index) {
      if (0 == encodingNameStr.compareNoCase(toString(types[index]))) return types[index];
    }
    return ReservedCodecPayloadType_Unknown;
  }

  //---------------------------------------------------------------------------
  ULONG IRTPTypes::getDefaultClockRate(ReservedCodecPayloadTypes reservedCodec)
  {
    switch (reservedCodec) {
      case ReservedCodecPayloadType_Unknown:      return 0;
      case ReservedCodecPayloadType_PCMU_8000:    return 8000;

      case ReservedCodecPayloadType_GSM_8000:     return 8000;
      case ReservedCodecPayloadType_G723_8000:    return 8000;
      case ReservedCodecPayloadType_DVI4_8000:    return 8000;
      case ReservedCodecPayloadType_DVI4_16000:   return 16000;
      case ReservedCodecPayloadType_LPC_8000:     return 8000;
      case ReservedCodecPayloadType_PCMA_8000:    return 8000;
      case ReservedCodecPayloadType_G722_8000:    return 8000;
      case ReservedCodecPayloadType_L16_44100_2:  return 44100;
      case ReservedCodecPayloadType_L16_44100_1:  return 44100;
      case ReservedCodecPayloadType_QCELP_8000:   return 8000;
      case ReservedCodecPayloadType_CN_8000:      return 8000;
      case ReservedCodecPayloadType_MPA_90000:    return 90000;
      case ReservedCodecPayloadType_G728_8000:    return 8000;
      case ReservedCodecPayloadType_DVI4_11025:   return 11025;
      case ReservedCodecPayloadType_DVI4_22050:   return 22050;
      case ReservedCodecPayloadType_G729_8000:    return 8000;

      case ReservedCodecPayloadType_CelB_90000:   return 90000;
      case ReservedCodecPayloadType_JPEG_90000:   return 90000;

      case ReservedCodecPayloadType_nv_90000:     return 90000;

      case ReservedCodecPayloadType_H261_90000:   return 90000;
      case ReservedCodecPayloadType_MPV_90000:    return 90000;
      case ReservedCodecPayloadType_MP2T_90000:   return 90000;
      case ReservedCodecPayloadType_H263_90000:   return 90000;
    }
    
    return 0;
  }

  //---------------------------------------------------------------------------
  IRTPTypes::CodecKinds IRTPTypes::getCodecKind(ReservedCodecPayloadTypes reservedCodec)
  {
    switch (reservedCodec) {
      case ReservedCodecPayloadType_Unknown:      return CodecKind_Unknown;
      case ReservedCodecPayloadType_PCMU_8000:    return CodecKind_Audio;

      case ReservedCodecPayloadType_GSM_8000:     return CodecKind_Audio;
      case ReservedCodecPayloadType_G723_8000:    return CodecKind_Audio;
      case ReservedCodecPayloadType_DVI4_8000:    return CodecKind_Audio;
      case ReservedCodecPayloadType_DVI4_16000:   return CodecKind_Audio;
      case ReservedCodecPayloadType_LPC_8000:     return CodecKind_Audio;
      case ReservedCodecPayloadType_PCMA_8000:    return CodecKind_Audio;
      case ReservedCodecPayloadType_G722_8000:    return CodecKind_Audio;
      case ReservedCodecPayloadType_L16_44100_2:  return CodecKind_Audio;
      case ReservedCodecPayloadType_L16_44100_1:  return CodecKind_Audio;
      case ReservedCodecPayloadType_QCELP_8000:   return CodecKind_Audio;
      case ReservedCodecPayloadType_CN_8000:      return CodecKind_Audio;
      case ReservedCodecPayloadType_MPA_90000:    return CodecKind_Video;
      case ReservedCodecPayloadType_G728_8000:    return CodecKind_Audio;
      case ReservedCodecPayloadType_DVI4_11025:   return CodecKind_Audio;
      case ReservedCodecPayloadType_DVI4_22050:   return CodecKind_Audio;
      case ReservedCodecPayloadType_G729_8000:    return CodecKind_Audio;

      case ReservedCodecPayloadType_CelB_90000:   return CodecKind_Audio;
      case ReservedCodecPayloadType_JPEG_90000:   return CodecKind_Video;

      case ReservedCodecPayloadType_nv_90000:     return CodecKind_Video;

      case ReservedCodecPayloadType_H261_90000:   return CodecKind_Video;
      case ReservedCodecPayloadType_MPV_90000:    return CodecKind_Video;
      case ReservedCodecPayloadType_MP2T_90000:   return CodecKind_AV;
      case ReservedCodecPayloadType_H263_90000:   return CodecKind_Video;
    }
    
    return CodecKind_Unknown;
  }

  //---------------------------------------------------------------------------
  IRTPTypes::SupportedCodecs IRTPTypes::toSupportedCodec(ReservedCodecPayloadTypes reservedCodec)
  {
    switch (reservedCodec) {
      case ReservedCodecPayloadType_Unknown:      return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_PCMU_8000:    return SupportedCodec_PCMU;

      case ReservedCodecPayloadType_GSM_8000:     return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_G723_8000:    return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_DVI4_8000:    return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_DVI4_16000:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_LPC_8000:     return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_PCMA_8000:    return SupportedCodec_PCMA;
      case ReservedCodecPayloadType_G722_8000:    return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_L16_44100_2:  return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_L16_44100_1:  return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_QCELP_8000:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_CN_8000:      return SupportedCodec_CN;
      case ReservedCodecPayloadType_MPA_90000:    return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_G728_8000:    return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_DVI4_11025:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_DVI4_22050:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_G729_8000:    return SupportedCodec_Unknown;

      case ReservedCodecPayloadType_CelB_90000:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_JPEG_90000:   return SupportedCodec_Unknown;

      case ReservedCodecPayloadType_nv_90000:     return SupportedCodec_Unknown;

      case ReservedCodecPayloadType_H261_90000:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_MPV_90000:    return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_MP2T_90000:   return SupportedCodec_Unknown;
      case ReservedCodecPayloadType_H263_90000:   return SupportedCodec_Unknown;
    }

    return SupportedCodec_Unknown;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::HeaderExtensionURIs
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(HeaderExtensionURIs extension)
  {
    switch (extension) {
      case HeaderExtensionURI_Unknown:                            return "";
      case HeaderExtensionURI_MuxID:                              return "urn:ietf:params:rtp-hdrext:sdes:mid";
      case HeaderExtensionURI_ClienttoMixerAudioLevelIndication:  return "urn:ietf:params:rtp-hdrext:ssrc-audio-level";
      case HeaderExtensionURI_MixertoClientAudioLevelIndication:  return "urn:ietf:params:rtp-hdrext:csrc-audio-level";
      case HeaderExtensionURI_FrameMarking:                       return "urn:ietf:params:rtp-hdrext:framemarkinginfo";
      case HeaderExtensionURI_RID:                                return "urn:ietf:params:rtp-hdrext:rid";
      case HeaderExtensionURI_3gpp_VideoOrientation:              return "urn:3gpp:video-orientation";
      case HeaderExtensionURI_3gpp_VideoOrientation6:             return "urn:3gpp:video-orientation:6";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::HeaderExtensionURIs IRTPTypes::toHeaderExtensionURI(const char *uri)
  {
    String uriStr(uri);

    for (HeaderExtensionURIs index = HeaderExtensionURI_First; index <= HeaderExtensionURI_Last; index = static_cast<HeaderExtensionURIs>(static_cast<std::underlying_type<HeaderExtensionURIs>::type>(index) + 1)) {
      if (uriStr == IRTPTypes::toString(index)) return index;
    }

    return HeaderExtensionURI_Unknown;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::KnownFeedbackTypes
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(KnownFECMechanisms mechanism)
  {
    switch (mechanism) {
      case KnownFECMechanism_Unknown:       return "";
      case KnownFECMechanism_RED:           return "red";
      case KnownFECMechanism_RED_ULPFEC:    return "red+ulpfec";
      case KnownFECMechanism_FLEXFEC:       return "flexfec";
    }

    return "unknown";  }

  //---------------------------------------------------------------------------
  IRTPTypes::KnownFECMechanisms IRTPTypes::toKnownFECMechanism(const char *mechanism)
  {
    String mechanismStr(mechanism);

    for (KnownFECMechanisms index = KnownFECMechanism_First; index <= KnownFECMechanism_Last; index = static_cast<KnownFECMechanisms>(static_cast<std::underlying_type<KnownFECMechanisms>::type>(index) + 1)) {
      if (mechanismStr == IRTPTypes::toString(index)) return index;
    }

    return KnownFECMechanism_Unknown;
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::KnownFeedbackTypes
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(KnownFeedbackTypes type)
  {
    switch (type) {
      case KnownFeedbackType_Unknown:               return "";

      case KnownFeedbackType_ACK:                   return "ack";
      case KnownFeedbackType_APP:                   return "app";
      case KnownFeedbackType_CCM:                   return "ccm";
      case KnownFeedbackType_NACK:                  return "nack";
      case KnownFeedbackType_TRR_INT:               return "trr-int";
      case KnownFeedbackType_3gpp_roi_arbitrary:    return "3gpp-roi-arbitrary";
      case KnownFeedbackType_3gpp_roi_predefined:   return "3gpp-roi-predefined";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::KnownFeedbackTypes IRTPTypes::toKnownFeedbackType(const char *type)
  {
    String typeStr(type);

    for (KnownFeedbackTypes index = KnownFeedbackType_First; index <= KnownFeedbackType_Last; index = static_cast<KnownFeedbackTypes>(static_cast<std::underlying_type<KnownFeedbackTypes>::type>(index) + 1)) {
      if (0 == typeStr.compareNoCase(IRTPTypes::toString(index))) return index;
    }

    return KnownFeedbackType_Unknown;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::KnownFeedbackMechanisms
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(KnownFeedbackMechanisms mechanism)
  {
    switch (mechanism) {
      case KnownFeedbackMechanism_Unknown:    return "";

      case KnownFeedbackMechanism_SLI:        return "sli";
      case KnownFeedbackMechanism_PLI:        return "pli";
      case KnownFeedbackMechanism_RPSI:       return "rpsi";
      case KnownFeedbackMechanism_APP:        return "app";
      case KnownFeedbackMechanism_RAI:        return "rai";
      case KnownFeedbackMechanism_TLLEI:      return "tllei";
      case KnownFeedbackMechanism_PSLEI:      return "pslei";
      case KnownFeedbackMechanism_FIR:        return "fir";
      case KnownFeedbackMechanism_TMMBR:      return "tmmbr";
      case KnownFeedbackMechanism_TSTR:       return "tstr";
      case KnownFeedbackMechanism_VBCM:       return "vbcm";
      case KnownFeedbackMechanism_PAUSE:      return "pause";
      case KnownFeedbackMechanism_REMB:       return "goog-remb";
    }

    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::KnownFeedbackMechanisms IRTPTypes::toKnownFeedbackMechanism(const char *mechanism)
  {
    String mechanismStr(mechanism);

    for (KnownFeedbackMechanisms index = KnownFeedbackMechanism_First; index <= KnownFeedbackMechanism_Last; index = static_cast<KnownFeedbackMechanisms>(static_cast<std::underlying_type<KnownFeedbackMechanisms>::type>(index) + 1)) {
      if (0 == mechanismStr.compareNoCase(IRTPTypes::toString(index))) return index;
    }

    return KnownFeedbackMechanism_Unknown;
  }

  //---------------------------------------------------------------------------
  IRTPTypes::KnownFeedbackTypesSet IRTPTypes::getUseableWithFeedbackTypes(KnownFeedbackMechanisms mechanism)
  {
    KnownFeedbackTypesSet result;

    switch (mechanism) {
      case KnownFeedbackMechanism_Unknown:    break;

      case KnownFeedbackMechanism_SLI:
      case KnownFeedbackMechanism_PLI:
      case KnownFeedbackMechanism_RAI:
      case KnownFeedbackMechanism_TLLEI:
      case KnownFeedbackMechanism_PSLEI:      {
        result.insert(KnownFeedbackType_NACK);
        break;
      }
      case KnownFeedbackMechanism_RPSI:
      case KnownFeedbackMechanism_APP:        {
        result.insert(KnownFeedbackType_ACK);
        result.insert(KnownFeedbackType_NACK);
        break;
      }
      case KnownFeedbackMechanism_REMB:
      case KnownFeedbackMechanism_FIR:
      case KnownFeedbackMechanism_TMMBR:
      case KnownFeedbackMechanism_TSTR:
      case KnownFeedbackMechanism_VBCM:
      case KnownFeedbackMechanism_PAUSE:      {
        result.insert(KnownFeedbackType_CCM);
        break;
      }
    }
    return result;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IRTPTypes::SupportedRTCPMechanisms
  #pragma mark

  //---------------------------------------------------------------------------
  const char *IRTPTypes::toString(SupportedRTCPMechanisms mechanism)
  {
    switch (mechanism) {
      case SupportedRTCPMechanism_Unknown:   return "";

      case SupportedRTCPMechanism_SR:        return "sr";
      case SupportedRTCPMechanism_RR:        return "rr";
      case SupportedRTCPMechanism_SDES:      return "sdes";
      case SupportedRTCPMechanism_BYE:       return "bye";
      case SupportedRTCPMechanism_RTPFB:     return "rtpfb";
      case SupportedRTCPMechanism_PSFB:      return "psfb";
    }
    return "unknown";
  }

  //---------------------------------------------------------------------------
  IRTPTypes::SupportedRTCPMechanisms IRTPTypes::toSupportedRTCPMechanism(const char *mechanism)
  {
    String mechanismStr(mechanism);

    for (SupportedRTCPMechanisms index = SupportedRTCPMechanism_First; index <= SupportedRTCPMechanism_Last; index = static_cast<SupportedRTCPMechanisms>(static_cast<std::underlying_type<SupportedRTCPMechanisms>::type>(index) + 1)) {
      if (0 == mechanismStr.compareNoCase(IRTPTypes::toString(index))) return index;
    }

    return SupportedRTCPMechanism_Unknown;
  }
}
