#include "cellref.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

ESM::CellRef::CellRef()
    : mScale(1.0f),
      mFactionRank(-2),
      mChargeInt(-1),
      mEnchantmentCharge(-1.0f),
      mGoldValue(1),
      mTeleport(false),
      mLockLevel(0),
      mReferenceBlocked(-1),
      mIsDeleted(false)
{}

void ESM::RefNum::load (ESMReader& esm, bool wide)
{
    if (wide)
        esm.getHNT (*this, "FRMR", 8);
    else
        esm.getHNT (mIndex, "FRMR");
}

void ESM::RefNum::save (ESMWriter &esm, bool wide, const std::string& tag) const
{
    if (wide)
        esm.writeHNT (tag, *this, 8);
    else
    {
        int refNum = (mIndex & 0xffffff) | ((hasContentFile() ? mContentFile : 0xff)<<24);

        esm.writeHNT (tag, refNum, 4);
    }
}


void ESM::CellRef::load (ESMReader& esm, bool wideRefNum)
{
    loadId(esm, wideRefNum);
    loadData(esm);
}

void ESM::CellRef::loadId (ESMReader& esm, bool wideRefNum)
{
    // According to Hrnchamd, this does not belong to the actual ref. Instead, it is a marker indicating that
    // the following refs are part of a "temp refs" section. A temp ref is not being tracked by the moved references system.
    // Its only purpose is a performance optimization for "immovable" things. We don't need this, and it's problematic anyway,
    // because any item can theoretically be moved by a script.
    if (esm.isNextSub ("NAM0"))
        esm.skipHSub();

    mRefNum.load (esm, wideRefNum);

    mRefID = esm.getHNString ("NAME");
    mIsDeleted = false;
}

void ESM::CellRef::loadData(ESMReader &esm)
{
    mScale = 1.0f;
    mFactionRank = -2;
    mChargeInt = -1;
    mEnchantmentCharge = -1;
    mGoldValue = 1;
    mLockLevel = 0;
    mReferenceBlocked = -1;
    mTeleport = false;
    mIsDeleted = false;

    bool isLoaded = false;
    while (!isLoaded && esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'U','N','A','M'>::value:
                esm.getHT(mReferenceBlocked);
                break;
            case ESM::FourCC<'X','S','C','L'>::value:
                esm.getHT(mScale);
                break;
            case ESM::FourCC<'A','N','A','M'>::value:
                mOwner = esm.getHString();
                break;
            case ESM::FourCC<'B','N','A','M'>::value:
                mGlobalVariable = esm.getHString();
                break;
            case ESM::FourCC<'X','S','O','L'>::value:
                mSoul = esm.getHString();
                break;
            case ESM::FourCC<'C','N','A','M'>::value:
                mFaction = esm.getHString();
                break;
            case ESM::FourCC<'I','N','D','X'>::value:
                esm.getHT(mFactionRank);
                break;
            case ESM::FourCC<'X','C','H','G'>::value:
                esm.getHT(mEnchantmentCharge);
                break;
            case ESM::FourCC<'I','N','T','V'>::value:
                esm.getHT(mChargeInt);
                break;
            case ESM::FourCC<'N','A','M','9'>::value:
                esm.getHT(mGoldValue);
                break;
            case ESM::FourCC<'D','O','D','T'>::value:
                esm.getHT(mDoorDest);
                mTeleport = true;
                break;
            case ESM::FourCC<'D','N','A','M'>::value:
                mDestCell = esm.getHString();
                break;
            case ESM::FourCC<'F','L','T','V'>::value:
                esm.getHT(mLockLevel);
                break;
            case ESM::FourCC<'K','N','A','M'>::value:
                mKey = esm.getHString();
                break;
            case ESM::FourCC<'T','N','A','M'>::value:
                mTrap = esm.getHString();
                break;
            case ESM::FourCC<'D','A','T','A'>::value:
                esm.getHT(mPos, 24);
                break;
            case ESM::FourCC<'N','A','M','0'>::value:
                esm.skipHSub();
                break;
            case ESM::FourCC<'D','E','L','E'>::value:
                esm.skipHSub();
                mIsDeleted = true;
                break;
            default:
                esm.cacheSubName();
                isLoaded = true;
                break;
        }
    }
}

void ESM::CellRef::save (ESMWriter &esm, bool wideRefNum, bool inInventory) const
{
    mRefNum.save (esm, wideRefNum);

    esm.writeHNCString("NAME", mRefID);

    if (mScale != 1.0) {
        esm.writeHNT("XSCL", mScale);
    }

    esm.writeHNOCString("ANAM", mOwner);
    esm.writeHNOCString("BNAM", mGlobalVariable);
    esm.writeHNOCString("XSOL", mSoul);

    esm.writeHNOCString("CNAM", mFaction);
    if (mFactionRank != -2) {
        esm.writeHNT("INDX", mFactionRank);
    }

    if (mEnchantmentCharge != -1)
        esm.writeHNT("XCHG", mEnchantmentCharge);

    if (mChargeInt != -1)
        esm.writeHNT("INTV", mChargeInt);

    if (mGoldValue != 1) {
        esm.writeHNT("NAM9", mGoldValue);
    }

    if (!inInventory && mTeleport)
    {
        esm.writeHNT("DODT", mDoorDest);
        esm.writeHNOCString("DNAM", mDestCell);
    }

    if (!inInventory && mLockLevel != 0) {
        esm.writeHNT("FLTV", mLockLevel);
    }

    if (!inInventory)
        esm.writeHNOCString ("KNAM", mKey);

    if (!inInventory)
        esm.writeHNOCString ("TNAM", mTrap);

    if (mReferenceBlocked != -1)
        esm.writeHNT("UNAM", mReferenceBlocked);

    if (!inInventory)
        esm.writeHNT("DATA", mPos, 24);

    if (mIsDeleted)
    {
        esm.writeHNCString("DELE", "");
    }
}

void ESM::CellRef::blank()
{
    mRefNum.unset();
    mRefID.clear();
    mScale = 1;
    mOwner.clear();
    mGlobalVariable.clear();
    mSoul.clear();
    mFaction.clear();
    mFactionRank = -2;
    mChargeInt = -1;
    mEnchantmentCharge = -1;
    mGoldValue = 0;
    mDestCell.clear();
    mLockLevel = 0;
    mKey.clear();
    mTrap.clear();
    mReferenceBlocked = -1;
    mTeleport = false;

    for (int i=0; i<3; ++i)
    {
        mDoorDest.pos[i] = 0;
        mDoorDest.rot[i] = 0;
        mPos.pos[i] = 0;
        mPos.rot[i] = 0;
    }

    mIsDeleted = false;
}

bool ESM::operator== (const RefNum& left, const RefNum& right)
{
    return left.mIndex==right.mIndex && left.mContentFile==right.mContentFile;
}

bool ESM::operator< (const RefNum& left, const RefNum& right)
{
    if (left.mIndex<right.mIndex)
        return true;

    if (left.mIndex>right.mIndex)
        return false;

    return left.mContentFile<right.mContentFile;
}
