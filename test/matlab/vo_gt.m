% load data
close all
clear
dir = '~/.ros';
dsvo_t = load(strcat(dir,'/time.txt'));
dsvo_t = dsvo_t(dsvo_t(:,1)==0,3);  % whole frame
gt = load(strcat(dir,'/truth.txt'));
dsvo = load(strcat(dir,'/vo.txt'));
dsvo = dsvo(:, [1,3,4,5]);
dsvo(:,1) = dsvo(:,1) + 0.15;
% gt = gt(500:end, :); dsvo = dsvo(500:end, :); sptam = sptam(500:end, :);
i = 1; % index of gt
while(gt(i,1) < dsvo(1,1))
    i = i+1;
end
gt = gt(i:end, :);

[gtp, dsvop, gtt, dsvot, gtvn, dsvovn, dsvo_diffo] = alignGT(gt, dsvo);
dsvo_diffn = abs(gtvn - dsvovn);
Method = 'DSVO';
scale_RMSE = sqrt(dsvo_diffn' * dsvo_diffn / size(dsvo_diffn,1)); 
scale_Median = median(dsvo_diffn);
direction_RMSE = sqrt(dsvo_diffo' * dsvo_diffo / size(dsvo_diffo,1));
direction_Median = median(dsvo_diffo);
time_Mean = mean(dsvo_t);

Result = table(Method, scale_RMSE, scale_Median, direction_RMSE, direction_Median, time_Mean);
disp(Result)

figure('Name','Trajectory (Top View)')
plot(dsvop(:,1), dsvop(:,2), 'g-')
hold on
plot(gtp(:,1), gtp(:,2), 'r-')
xlabel('x [m]');ylabel('y [m]');
legend('DSVO', 'Truth');
axis equal
view(90,90)

figure('Name', 'Position drift')
plot(gtt, vecnorm(dsvop'-gtp'), 'g-')
xlabel('Time [s]'); ylabel('position drift [m]');
% legend('DSVO');



% %% load data
% close all
% clear
% dir = '~/.ros';
% % dir = '/home/jiawei/Dropbox/results/mh1/stereo';
% % dir = '/home/jiawei/Dropbox/results/mh1/mono';
% % dir = '/home/jiawei/Dropbox/results/mh1/dsvo';
% % dir = '/home/jiawei/Dropbox/results/mh1/sptam';
% gt_a = load(strcat(dir,'/truth.txt'));
% vo = load(strcat(dir,'/vo.txt'));
% % vo(:,1) = vo(:,1) + 0.15;
% % gt_a = gt_a(1:end-80, :);
% % vo = vo(1:end-80, :);
% % gt_a = gt_a(15:end, :);
% % vo = vo(15:end, :);
% 
% % get overlap of gt with vo
% i = 1; % index of vo
% while(vo(i,1) < gt_a(1,1))
%     i = i+1;
% end
% vo = vo(i:end, :);
% i = 1;
% j = 1; % index of gt_a
% k = 1; % index of gt_a
% while i<=size(vo,1)
%     while(j<=size(gt_a,1) && vo(i,1) >= gt_a(j,1))
%         j = j+1;
%     end
%     if(j>size(gt_a,1))
%         break;
%     else
%         gt(k,1) = vo(i,1);
%         inc = (gt_a(j,2:4) - gt_a(j-1,2:4))/(gt_a(j,1) - gt_a(j-1,1));
%         gt(k,2:4) = gt_a(j-1,2:4) + (vo(i,1) - gt_a(j-1,1))*inc;
%     end
%     i = i+1;
%     k = k+1;
% end
% vo = vo(1:length(gt),:);
% 
% %% align two set of points
% cg = mean(gt(:,2:4));
% cv = mean(vo(:,3:5));
% H = zeros(3,3);
% for i=1:length(gt)
%     H = H + (gt(i,2:4)-cg)'*(vo(i,3:5)-cv);
% end
% [U,S,V] = svd(H);
% R = U*V';
% for i=1:size(vo,1)
%     p = R*vo(i,3:5)';
%     vo(i,3:5) = p';
% end
% cg = mean(gt(:,2:4));
% cv = mean(vo(:,3:5));
% for i=size(vo,1):-1:1
%     vo(i,3:5) = vo(i,3:5) - cv;
% end
% for i=size(gt,1):-1:1
%     gt(i,2:4) = gt(i,2:4) - cg;
% end
% 
% step = floor(length(vo) / (vo(end,1)-vo(1,1))); % step of 1 sec
% 
% % calculate vo translation
% for i=1+step:size(vo,1)
%     vo(i, 6:8) = (vo(i, 3:5) - vo(i-step, 3:5)) / (vo(i, 1) - vo(i-step, 1));
% end
% vo = vo(1+step:end, :);
% di = vo(:,2)==0;
% si = vo(:,2)==1;
% 
% % calculate gt translation
% for i=1+step:size(gt,1)
%     gt(i, 5:7) = (gt(i, 2:4) - gt(i-step, 2:4)) / (gt(i, 1) - gt(i-step, 1));
% end
% gt = gt(1+step:end, :);
% 
% 
% %% plot results
% % position
% gtp = gt(:,2:4);
% vop_d = vo(di,3:5);
% vop_s = vo(si,3:5);
% figure('Name','Trajectory')
% plot3(gtp(:,1), gtp(:,2), gtp(:,3), 'g.')
% hold on
% plot3(vop_d(:,1), vop_d(:,2), vop_d(:,3), 'r.')
% hold on
% plot3(vop_s(:,1), vop_s(:,2), vop_s(:,3), 'b.')
% axis equal
% legend('Truth', 'Estimated');
% title('Trajectory')
% 
% % velocity scale
% gtvn = vecnorm(gt(:,5:7)')';
% vovn = vecnorm(vo(:,6:8)')';
% diffn = abs(gtvn - vovn);
% diffn_d = diffn(di);
% diffn_s = diffn(si);
% fprintf('RMSE offset of scale (DSVO, Stereo, Overall) = (%f, %f, %f)\n', ...
%     sqrt(diffn_d' * diffn_d / length(diffn_d)), sqrt(diffn_s' * diffn_s / length(diffn_s)), sqrt(diffn' * diffn / length(diffn)));
% fprintf('Median offset of scale (DSVO, Stereo, Overall) = (%f, %f, %f)\n', median(diffn_d), median(diffn_s), median(diffn));
% 
% 
% figure('Name','Velocity Scale')
% subplot(2,1,1);
% plot(gt(:,1), gtvn, 'g.');
% hold on
% plot(vo(di,1), vovn(di), 'r.');
% hold on
% plot(vo(si,1), vovn(si), 'b.');
% legend('Truth', 'Estimated by DSVO', 'Estimated by Stereo Match');
% xlabel('Time s'); ylabel('Velocity m/s');
% subplot(2,1,2);
% plot(vo(di,1), diffn(di), 'r.');
% hold on 
% plot(vo(si,1), diffn(si), 'b.');
% legend('DSVO', 'Stereo Match');
% xlabel('Time s'); ylabel('Velocity Error m/s');
% 
% % % velocity orientation
% % gtvd = gt(:,5:7) ./ gtvn;
% % vovd = vo(:,6:8) ./ vovn;
% % gt_vel_ori = [gt(:,1), gt(:,1), gt(:,1)];
% % vo_vel_ori = [vo(:,1), vo(:,1), vo(:,1)];
% % for i=2:size(vo,1)
% %     gt_vel_ori(i,:) = acos( gtvd(i,:))/3.14159*180;
% %     vo_vel_ori(i,:) = acos( vovd(i,:))/3.14159*180;
% % end
% % figure('Name','Velocity Direction');
% % subplot(3,1,1);
% % plot(gt(:,1), gt_vel_ori(:,1), 'g-');
% % hold on
% % plot(vo(:,1), vo_vel_ori(:,1), 'r-');
% % title('x direction');
% % legend('Truth', 'Estimated');
% % subplot(3,1,2);
% % plot(gt(:,1), gt_vel_ori(:,2), 'g-');
% % hold on
% % plot(vo(:,1), vo_vel_ori(:,2), 'r-');
% % title('y direction');
% % subplot(3,1,3);
% % plot(gt(:,1), gt_vel_ori(:,3), 'g-');
% % hold on
% % plot(vo(:,1), vo_vel_ori(:,3), 'r-');
% % title('z direction');
% 
% 
