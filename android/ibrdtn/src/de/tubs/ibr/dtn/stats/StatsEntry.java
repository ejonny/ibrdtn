package de.tubs.ibr.dtn.stats;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.annotation.SuppressLint;
import android.content.Context;
import android.database.Cursor;
import android.provider.BaseColumns;
import android.util.Log;
import de.tubs.ibr.dtn.api.Timestamp;
import de.tubs.ibr.dtn.swig.NativeStats;

public class StatsEntry {
    
    private final static String TAG = "StatsEntry";
    
    public static final String ID = BaseColumns._ID;
    public static final String TIMESTAMP = "timestamp";
    
    public static final String UPTIME = "uptime";
    public static final String NEIGHBORS = "neighbors";
    public static final String STORAGE_SIZE = "storage_size";
    
    public static final String CLOCK_OFFSET = "clock_offset";
    public static final String CLOCK_RATING = "clock_rating";
    public static final String CLOCK_ADJUSTMENTS = "clock_adjustments";
    
    public static final String BUNDLE_ABORTED = "bundle_aborted";
    public static final String BUNDLE_EXPIRED = "bundle_expired";
    public static final String BUNDLE_GENERATED = "bundle_generated";
    public static final String BUNDLE_QUEUED = "bundle_queued";
    public static final String BUNDLE_RECEIVED = "bundle_received";
    public static final String BUNDLE_REQUEUED = "bundle_requeued";
    public static final String BUNDLE_STORED = "bundle_stored";
    public static final String BUNDLE_TRANSMITTED = "bundle_transmitted";
    
    private Long mId = null;
    private Date mTimestamp = null;
    private Long mUptime = null;
    private Long mNeighbors = null;
    private Long mStorageSize = null;
    private Double mClockOffset = null;
    private Double mClockRating = null;
    private Long mClockAdjustments = null;
    private Long mBundleAborted = null;
    private Long mBundleExpired = null;
    private Long mBundleGenerated = null;
    private Long mBundleQueued = null;
    private Long mBundleReceived = null;
    private Long mBundleRequeued = null;
    private Long mBundleStored = null;
    private Long mBundleTransmitted = null;
    
    public static final String[] PROJECTION = new String[] {
        BaseColumns._ID,
        StatsEntry.TIMESTAMP,
        StatsEntry.UPTIME,
        StatsEntry.NEIGHBORS,
        StatsEntry.STORAGE_SIZE,
        StatsEntry.CLOCK_OFFSET,
        StatsEntry.CLOCK_RATING,
        StatsEntry.CLOCK_ADJUSTMENTS,
        StatsEntry.BUNDLE_ABORTED,
        StatsEntry.BUNDLE_EXPIRED,
        StatsEntry.BUNDLE_GENERATED,
        StatsEntry.BUNDLE_QUEUED,
        StatsEntry.BUNDLE_RECEIVED,
        StatsEntry.BUNDLE_REQUEUED,
        StatsEntry.BUNDLE_STORED,
        StatsEntry.BUNDLE_TRANSMITTED
    };
    
    // The indexes of the default columns which must be consistent
    // with above PROJECTION.
    static final int COLUMN_STATS_ID                 = 0;
    static final int COLUMN_STATS_TIMESTAMP          = 1;
    
    static final int COLUMN_STATS_UPTIME             = 2;
    static final int COLUMN_STATS_NEIGHBORS          = 3;
    static final int COLUMN_STATS_STORAGE_SIZE       = 4;
    
    static final int COLUMN_STATS_CLOCK_OFFSET       = 5;
    static final int COLUMN_STATS_CLOCK_RATING       = 6;
    static final int COLUMN_STATS_CLOCK_ADJUSTMENTS  = 7;
    
    static final int COLUMN_STATS_BUNDLE_ABORTED     = 8;
    static final int COLUMN_STATS_BUNDLE_EXPIRED     = 9;
    static final int COLUMN_STATS_BUNDLE_GENERATED   = 10;
    static final int COLUMN_STATS_BUNDLE_QUEUED      = 11;
    static final int COLUMN_STATS_BUNDLE_RECEIVED    = 12;
    static final int COLUMN_STATS_BUNDLE_REQUEUED    = 13;
    static final int COLUMN_STATS_BUNDLE_STORED      = 14;
    static final int COLUMN_STATS_BUNDLE_TRANSMITTED = 15;

    @SuppressLint("SimpleDateFormat")
    public StatsEntry(Context context, Cursor cursor, ColumnsMap cmap) {
        final DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        
        mId = cursor.getLong(cmap.mColumnId);
        
        mNeighbors = cursor.getLong(cmap.mColumnNeighbors);
        mUptime = cursor.getLong(cmap.mColumnUptime);
        mStorageSize = cursor.getLong(cmap.mColumnStorageSize);
        mClockOffset = cursor.getDouble(cmap.mColumnClockOffset);
        mClockRating = cursor.getDouble(cmap.mColumnClockRating);
        mClockAdjustments = cursor.getLong(cmap.mColumnClockAdjustments);
        mBundleAborted = cursor.getLong(cmap.mColumnBundleAborted);
        mBundleExpired = cursor.getLong(cmap.mColumnBundleExpired);
        mBundleGenerated = cursor.getLong(cmap.mColumnBundleGenerated);
        mBundleQueued = cursor.getLong(cmap.mColumnBundleQueued);
        mBundleReceived = cursor.getLong(cmap.mColumnBundleReceived);
        mBundleRequeued = cursor.getLong(cmap.mColumnBundleRequeued);
        mBundleStored = cursor.getLong(cmap.mColumnBundleStored);
        mBundleTransmitted = cursor.getLong(cmap.mColumnBundleTransmitted);
        
        try {
            mTimestamp = formatter.parse(cursor.getString(cmap.mColumnTimestamp));
        } catch (ParseException e) {
            Log.e(TAG, "failed to convert date");
        }
    }
    
    public StatsEntry(NativeStats stats) {
        mTimestamp = (new Timestamp(stats.getTimestamp())).getDate();
        mNeighbors = stats.getNeighbors();
        mUptime = stats.getUptime();
        mStorageSize = stats.getStorage_size();
        mClockOffset = stats.getTime_offset();
        mClockRating = stats.getTime_rating();
        mClockAdjustments = stats.getTime_adjustments();
        mBundleAborted = stats.getBundles_aborted();
        mBundleExpired = stats.getBundles_expired();
        mBundleGenerated = stats.getBundles_generated();
        mBundleQueued = stats.getBundles_queued();
        mBundleReceived = stats.getBundles_received();
        mBundleRequeued = stats.getBundles_requeued();
        mBundleStored = stats.getBundles_stored();
        mBundleTransmitted = stats.getBundles_transmitted();
    }
    
    public Long getId() {
        return mId;
    }
    
    public Date getTimestamp() {
        return mTimestamp;
    }

    public void setTimestamp(Date timestamp) {
        mTimestamp = timestamp;
    }
    
    public Long getNeighbors() {
        return mNeighbors;
    }

    public void setNeighbors(Long neighbors) {
        mNeighbors = neighbors;
    }

    public Double getClockOffset() {
        return mClockOffset;
    }

    public void setClockOffset(Double clockOffset) {
        mClockOffset = clockOffset;
    }

    public Double getClockRating() {
        return mClockRating;
    }

    public void setClockRating(Double clockRating) {
        mClockRating = clockRating;
    }

    public Long getClockAdjustments() {
        return mClockAdjustments;
    }

    public void setClockAdjustments(Long clockAdjustments) {
        mClockAdjustments = clockAdjustments;
    }

    public Long getBundleAborted() {
        return mBundleAborted;
    }

    public void setBundleAborted(Long bundleAborted) {
        mBundleAborted = bundleAborted;
    }

    public Long getBundleExpired() {
        return mBundleExpired;
    }

    public void setBundleExpired(Long bundleExpired) {
        mBundleExpired = bundleExpired;
    }

    public Long getBundleGenerated() {
        return mBundleGenerated;
    }

    public void setBundleGenerated(Long bundleGenerated) {
        mBundleGenerated = bundleGenerated;
    }

    public Long getBundleQueued() {
        return mBundleQueued;
    }

    public void setBundleQueued(Long bundleQueued) {
        mBundleQueued = bundleQueued;
    }

    public Long getBundleReceived() {
        return mBundleReceived;
    }

    public void setBundleReceived(Long bundleReceived) {
        mBundleReceived = bundleReceived;
    }

    public Long getBundleRequeued() {
        return mBundleRequeued;
    }

    public void setBundleRequeued(Long bundleRequeued) {
        mBundleRequeued = bundleRequeued;
    }

    public Long getBundleStored() {
        return mBundleStored;
    }

    public void setBundleStored(Long bundleStored) {
        mBundleStored = bundleStored;
    }

    public Long getBundleTransmitted() {
        return mBundleTransmitted;
    }

    public void setBundleTransmitted(Long bundleTransmitted) {
        mBundleTransmitted = bundleTransmitted;
    }

    public Long getUptime() {
        return mUptime;
    }

    public void setUptime(Long uptime) {
        mUptime = uptime;
    }

    public Long getStorageSize() {
        return mStorageSize;
    }

    public void setStorageSize(Long storageSize) {
        mStorageSize = storageSize;
    }

    public static class ColumnsMap {
        public int mColumnId;
        public int mColumnTimestamp;
        public int mColumnNeighbors;
        public int mColumnStorageSize;
        public int mColumnUptime;
        public int mColumnClockOffset;
        public int mColumnClockRating;
        public int mColumnClockAdjustments;
        public int mColumnBundleAborted;
        public int mColumnBundleExpired;
        public int mColumnBundleGenerated;
        public int mColumnBundleQueued;
        public int mColumnBundleReceived;
        public int mColumnBundleRequeued;
        public int mColumnBundleStored;
        public int mColumnBundleTransmitted;

        public ColumnsMap() {
            mColumnTimestamp   = COLUMN_STATS_TIMESTAMP;
            mColumnNeighbors = COLUMN_STATS_NEIGHBORS;
            mColumnUptime = COLUMN_STATS_UPTIME;
            mColumnStorageSize = COLUMN_STATS_STORAGE_SIZE;
            mColumnClockOffset = COLUMN_STATS_CLOCK_OFFSET;
            mColumnClockRating = COLUMN_STATS_CLOCK_RATING;
            mColumnClockAdjustments = COLUMN_STATS_CLOCK_ADJUSTMENTS;
            mColumnBundleAborted = COLUMN_STATS_BUNDLE_ABORTED;
            mColumnBundleExpired = COLUMN_STATS_BUNDLE_EXPIRED;
            mColumnBundleGenerated = COLUMN_STATS_BUNDLE_GENERATED;
            mColumnBundleQueued = COLUMN_STATS_BUNDLE_QUEUED;
            mColumnBundleReceived = COLUMN_STATS_BUNDLE_RECEIVED;
            mColumnBundleRequeued = COLUMN_STATS_BUNDLE_REQUEUED;
            mColumnBundleStored = COLUMN_STATS_BUNDLE_STORED;
            mColumnBundleTransmitted = COLUMN_STATS_BUNDLE_TRANSMITTED;
        }

        public ColumnsMap(Cursor cursor) {
            // Ignore all 'not found' exceptions since the custom columns
            // may be just a subset of the default columns.
            try {
                mColumnId = cursor.getColumnIndexOrThrow(BaseColumns._ID);
            } catch (IllegalArgumentException e) {
                Log.w("colsMap", e.getMessage());
            }

            try {
                mColumnTimestamp = cursor.getColumnIndexOrThrow(StatsEntry.TIMESTAMP);
            } catch (IllegalArgumentException e) {
                Log.w("colsMap", e.getMessage());
            }
        }
    }
}
