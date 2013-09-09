package de.tubs.ibr.dtn.stats;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.support.v4.content.AsyncTaskLoader;
import android.util.Log;
import de.tubs.ibr.dtn.service.DaemonService;

@SuppressLint("SimpleDateFormat")
public class StatsLoader extends AsyncTaskLoader<Cursor> {
    
    private static final String TAG = "StatsLoader";
    
    private DaemonService mService = null;
    private Boolean mStarted = false;
    private Cursor mData = null;

    public StatsLoader(Context context, DaemonService service) {
        super(context);
        mService = service;
        setUpdateThrottle(250);
    }
    
    @Override
    public void deliverResult(Cursor data) {
        if (isReset()) {
            mData = null;
            return;
        }
        
        mData = data;
        
        if (isStarted()) {
            super.deliverResult(data);
        }
    }

    @Override
    protected void onReset() {
        onStopLoading();
        
        if (mStarted) {
            // unregister from intent receiver
            getContext().unregisterReceiver(_receiver);
            mStarted = false;
        }
    }

    @Override
    protected void onStartLoading() {
        if (mData != null) {
            this.deliverResult(mData);
        }
        
        IntentFilter filter = new IntentFilter(StatsDatabase.NOTIFY_DATABASE_UPDATED);
        filter.addCategory(Intent.CATEGORY_DEFAULT);
        getContext().registerReceiver(_receiver, filter);
        mStarted = true;
        
        if (this.takeContentChanged() || mData == null) {
            forceLoad();
        }
    }

    @Override
    protected void onStopLoading() {
        cancelLoad();
    }

    @Override
    public Cursor loadInBackground() {
        SQLiteDatabase db = mService.getStatsDatabase().getDB();
        
        final DateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        
        // generate a time limit (24 hours)
        Calendar cal = Calendar.getInstance();
        cal.roll(Calendar.DATE, -1);
        String timestamp_limit = formatter.format(cal.getTime());

        try {
            // limit to specific download
            return db.query(
                    StatsDatabase.TABLE_NAMES[0],
                    StatsEntry.PROJECTION,
                    StatsEntry.TIMESTAMP + " >= ?",
                    new String[] { timestamp_limit },
                    null, null, StatsEntry.TIMESTAMP + " ASC");
        } catch (Exception e) {
            Log.e(TAG, "loadInBackground() failed", e);
        }
        
        return null;
    }

    private BroadcastReceiver _receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            onContentChanged();
        }
    };
}
